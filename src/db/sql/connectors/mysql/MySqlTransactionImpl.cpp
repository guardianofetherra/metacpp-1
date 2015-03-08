/****************************************************************************
* Copyright 2014-2015 Trefilov Dmitrij                                      *
*                                                                           *
* Licensed under the Apache License, Version 2.0 (the "License");           *
* you may not use this file except in compliance with the License.          *
* You may obtain a copy of the License at                                   *
*                                                                           *
*    http://www.apache.org/licenses/LICENSE-2.0                             *
*                                                                           *
* Unless required by applicable law or agreed to in writing, software       *
* distributed under the License is distributed on an "AS IS" BASIS,         *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
* See the License for the specific language governing permissions and       *
* limitations under the License.                                            *
****************************************************************************/
#include "MySqlTransactionImpl.h"

namespace metacpp {
namespace db {
namespace sql {
namespace connectors {
namespace mysql {

MySqlTransactionImpl::MySqlTransactionImpl(MYSQL *dbConn)
    : m_dbConn(dbConn)
{

}

MySqlTransactionImpl::~MySqlTransactionImpl()
{
    std::lock_guard<std::mutex> _guard(m_statementsMutex);
    if (m_statements.size())
    {
        std::cerr << "There's still " << m_statements.size() <<
                     " unclosed statements while destroing the postgres transaction" << std::endl;
    }
}

bool MySqlTransactionImpl::begin()
{
    return execCommand("BEGIN", "MySqlTransactionImpl::begin()");
}

bool MySqlTransactionImpl::commit()
{
    return execCommand("COMMIT", "MySqlTransactionImpl::commit()");
}

bool MySqlTransactionImpl::rollback()
{
    return execCommand("ROLLBACK", "MySqlTransactionImpl::rollback()");
}

SqlStatementImpl *MySqlTransactionImpl::createStatement(SqlStatementType type, const String& queryText)
{
    std::lock_guard<std::mutex> _guard(m_statementsMutex);
    MYSQL_STMT *stmt = mysql_stmt_init(dbConn());
    if (!stmt)
    {
        std::cerr << "mysql_stmt_init() failed:" << mysql_error(dbConn());
        return nullptr;
    }
    MySqlStatementImpl *statement = new MySqlStatementImpl(stmt, type, queryText);
    m_statements.push_back(statement);
    return statement;
}

bool MySqlTransactionImpl::prepare(SqlStatementImpl *statement)
{
    MySqlStatementImpl *mysqlStatement = reinterpret_cast<MySqlStatementImpl *>(statement);
    int res = mysql_stmt_prepare(mysqlStatement->getStmt(), statement->queryText().c_str(), statement->queryText().length());
    if (0 != res)
    {
        std::cerr << "mysql_stmt_prepare() failed: " << mysql_error(dbConn());
        return false;
    }
    statement->setPrepared();
    return true;
}

bool MySqlTransactionImpl::execStatement(SqlStatementImpl *statement, int *numRowsAffected)
{
    if (!statement->prepared())
        throw std::runtime_error("MySqlTransactionImpl::execStatement(): should be prepared first");
    MySqlStatementImpl *mysqlStatement = reinterpret_cast<MySqlStatementImpl *>(statement);
    int res = mysql_stmt_execute(mysqlStatement->getStmt());
    if (0 != res)
    {
        std::cerr << "mysql_stmt_execute() failed: " << mysql_error(dbConn());
        return false;
    }
    if (numRowsAffected) *numRowsAffected = mysql_stmt_affected_rows(mysqlStatement->getStmt());
    return true;
}

template<typename T>
void assignField(const MetaFieldBase *field, bool isNull, const T& val, Object *obj)
{
    if (field->nullable() && !isNull) {
        field->access<Nullable<T> >(obj).reset();
    } else {
        if (field->nullable())
            field->access<Nullable<T> >(obj) = val;
        else
            field->access<T>(obj) = val;
    }
}

void fieldTypeToMysqlType(EFieldType type, enum_field_types& mysqlType, my_bool& isUnsigned)
{
    isUnsigned = false;
    switch (type)
    {
    case eFieldBool:
        mysqlType = MYSQL_TYPE_TINY;
        break;
    case eFieldUint:
        isUnsigned = true; // fall through
    case eFieldInt:
    case eFieldEnum:
        mysqlType = MYSQL_TYPE_LONG;
        break;
    case eFieldUint64:
    case eFieldInt64:
        mysqlType = MYSQL_TYPE_LONGLONG;
        break;
    case eFieldFloat:
        mysqlType = MYSQL_TYPE_FLOAT;
        break;
    case eFieldDouble:
        mysqlType = MYSQL_TYPE_DOUBLE;
        break;
    case eFieldString:
        mysqlType = MYSQL_TYPE_STRING;
        break;
    case eFieldDateTime:
        mysqlType = MYSQL_TYPE_DATETIME;
        break;
    default:
        mysqlType = (enum_field_types)-1;
    }

}

bool MySqlTransactionImpl::fetchNext(SqlStatementImpl *statement, SqlStorable *storable)
{
    if (!statement->prepared())
        throw std::runtime_error("MySqlTransactionImpl::execStatement(): should be prepared first");
    if (statement->done())
        return false;
    MySqlStatementImpl *mysqlStatement = reinterpret_cast<MySqlStatementImpl *>(statement);
    if (!mysqlStatement->getExecuted())
    {
        int execRes = mysql_stmt_execute(mysqlStatement->getStmt());
        if (0 != execRes)
            throw std::runtime_error(std::string() + "mysql_stmt_execute() failed: " + mysql_error(dbConn()));
        mysqlStatement->setExecuted();
    }

    MYSQL_RES * res = mysqlStatement->getResult();
    if (!res)
    {
        res = mysql_stmt_result_metadata(mysqlStatement->getStmt());
        if (!res)
            throw std::runtime_error(std::string() + "mysql_stmt_result_metadata() failed: " + mysql_error(dbConn()));
        mysqlStatement->setResult(res);
    }
    unsigned int nFields = mysql_num_fields(res);
    mysqlStatement->prefetch();
    int fetchRes = mysql_stmt_fetch(mysqlStatement->getStmt());
    if (MYSQL_NO_DATA == fetchRes)
    {
        mysqlStatement->setDone();
        return false;
    }
    if (0 != fetchRes)
        throw std::runtime_error(std::string() + "mysql_stmt_fetch() failed: " + mysql_error(dbConn()));

    for (unsigned int i = 0; i < nFields; ++i)
    {
        MYSQL_FIELD *mysqlField = mysql_fetch_field_direct(res, i);
        auto field = storable->record()->metaObject()->fieldByName(String(mysqlField->name, mysqlField->name_length), false);
        if (!field)
        {
            std::cerr << "Cannot bind sql result to an object field " << field->name() << std::endl;
            continue;
        }

        if (mysqlStatement->bindResult(i)->is_null)
        {
            if (!field->nullable()) throw std::runtime_error(std::string() + "Field " + field->name() + " is not nullable");
            field->setValue(Variant(), storable->record());
        }
        MYSQL_BIND *bind = mysqlStatement->bindResult(i);
#pragma pack(push, 1)
        struct MYSQL_DATETIME {
            int sign:1;
            int year_month:17;

        };
#pragma pack(pop)
        MYSQL_TIME timeBuffer;

        switch (field->type())
        {
        case eFieldBool:
        case eFieldInt:
        case eFieldEnum:
        case eFieldUint:
        case eFieldInt64:
        case eFieldUint64:
        case eFieldFloat:
        case eFieldDouble:
            bind->buffer = reinterpret_cast<char *>(storable->record()) + field->offset();
            fieldTypeToMysqlType(field->type(), bind->buffer_type, bind->is_unsigned);
            break;
        case eFieldString:
            bind->buffer_length = *bind->length;
            field->access<String>(storable->record()).resize(*bind->length);
            bind->buffer = const_cast<char *>(field->access<String>(storable->record()).data());
            break;
        case eFieldDateTime:
            bind->buffer_length = sizeof(timeBuffer);
            bind->buffer = &timeBuffer;
            bind->buffer_type = MYSQL_TYPE_DATETIME;
            break;
        case eFieldObject:
        case eFieldArray:
            throw std::runtime_error("Cannot handle non-plain objects");
        default:
            throw std::runtime_error("Unknown field type");
        }
        int fetchRes = mysql_stmt_fetch_column(mysqlStatement->getStmt(), bind, i, 0);
        if (0 != fetchRes)
            throw std::runtime_error(std::string() + "mysql_stmt_fetch_column() failed:" + mysql_error(dbConn()));
        if (eFieldDateTime == field->type())
            field->access<DateTime>(storable->record()) = DateTime(timeBuffer.year,
                static_cast<EMonth>(timeBuffer.month - 1), timeBuffer.day, timeBuffer.hour,
                timeBuffer.minute, timeBuffer.second);
    }
    return true;
}

bool MySqlTransactionImpl::getLastInsertId(SqlStatementImpl *statement, SqlStorable *storable)
{
    (void)statement;
    auto pkey = storable->primaryKey();
    if (!pkey) return false;
    pkey->setValue((uint64_t)mysql_insert_id(dbConn()), storable->record());

    return true;
}

bool MySqlTransactionImpl::closeStatement(SqlStatementImpl *statement)
{
    std::lock_guard<std::mutex> _guard(m_statementsMutex);
    MySqlStatementImpl *mysqlStatement = reinterpret_cast<MySqlStatementImpl *>(statement);
    auto it = std::find(m_statements.begin(), m_statements.end(), mysqlStatement);
    if (it == m_statements.end())
    {
        std::cerr << "MySqlTransactionImpl::closeStatement(): there's no such statement" << std::endl;
        return false;
    }
    m_statements.erase(it);
    delete mysqlStatement;
    return true;
}

bool MySqlTransactionImpl::execCommand(const char *query, const char *invokeContext)
{
    int res = mysql_query(dbConn(), query);
    if (0 != res)
    {
        std::cerr << invokeContext << ": mysql_query() failed: " << mysql_error(dbConn());
        return false;
    }
    return true;
}

} // namespace mysql
} // namespace connectors
} // namespace sql
} // namespace db
} // namespace metacpp
