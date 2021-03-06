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
#include "JsonDeserializerVisitor.h"
#include "Object.h"
#include <cassert>
#include <time.h>

namespace metacpp
{
namespace serialization
{
namespace json
{

static const char szTypeName[] = "@type";

JsonDeserializerVisitor::JsonDeserializerVisitor(const Json::Value& val,
    TypeResolverFactory *typeResolver)
    : m_value(val), m_typeResolver(typeResolver)
{
}


JsonDeserializerVisitor::~JsonDeserializerVisitor(void)
{
}

void JsonDeserializerVisitor::visitField(Object *obj, const MetaFieldBase *field)
{
    parseValue(m_value.get(field->name(), Json::Value()),
            field->type(),
            reinterpret_cast<char *>(obj) + field->offset(),
            field);
}

template<typename T>
static void *DerefernceNullable(Nullable<T> *pValue, bool isNull)
{
    auto& f = *pValue;
    f.reset(!isNull);
    if (isNull)
        return nullptr;
    return &f.get();
}

static metacpp::Variant ToVariant(const Json::Value& val, TypeResolverFactory *typeResolver)
{
    switch (val.type()){
    case Json::nullValue:
        return metacpp::Variant();
    case Json::intValue:
        return static_cast<int32_t>(val.asInt());
    case Json::uintValue:
        return static_cast<uint32_t>(val.asUInt());
    case Json::realValue:
        return static_cast<double>(val.asDouble());
    case Json::stringValue:
        return static_cast<String>(val.asString());
    case Json::booleanValue:
        return static_cast<bool>(val.asBool());
    case Json::arrayValue: {
        metacpp::VariantArray result;
        result.reserve(val.size());
        for (size_t i = 0; i < val.size(); ++i)
            result.push_back(ToVariant(val.get((Json::ArrayIndex)i, Json::Value()), typeResolver));
        return result;
    }
    case Json::objectValue: {
        Json::Value typeValue = val[szTypeName];
        if (!typeValue.isString())
            throw std::invalid_argument("Unknown type of Object encapsulated into Variant");
        auto typeName = typeValue.asString();
        if (!typeResolver)
            throw std::invalid_argument("Unknown type " + typeName);
        Object *subObj = typeResolver->createInstance(typeValue.asString());
        JsonDeserializerVisitor visitor(val, typeResolver);
        visitor.visit(subObj);
        return subObj;
    }
    default:
        throw std::invalid_argument("Unknown value type " + std::to_string(val.type()));
    }
}

void JsonDeserializerVisitor::parseValue(const Json::Value& val, EFieldType type, void *pValue, const MetaFieldBase *field)
{
    if (field && field->nullable())
    {
        switch (type)
        {
        default:
        case eFieldVoid:
            throw std::invalid_argument(std::string("Unsupported nullable field type: ") + (char *)type);
        case eFieldBool:
            pValue = DerefernceNullable(reinterpret_cast<Nullable<bool> *>(pValue), val.isNull());
            break;
        case eFieldInt:
            pValue = DerefernceNullable(reinterpret_cast<Nullable<int32_t> *>(pValue), val.isNull());
            break;
        case eFieldEnum:
        case eFieldUint:
            pValue = DerefernceNullable(reinterpret_cast<Nullable<uint32_t> *>(pValue), val.isNull());
            break;
        case eFieldInt64:
            pValue = DerefernceNullable(reinterpret_cast<Nullable<int64_t> *>(pValue), val.isNull());
            break;
        case eFieldUint64:
            pValue = DerefernceNullable(reinterpret_cast<Nullable<uint64_t> *>(pValue), val.isNull());
            break;
        case eFieldFloat:
            pValue = DerefernceNullable(reinterpret_cast<Nullable<float> *>(pValue), val.isNull());
            break;
        case eFieldDouble:
            pValue = DerefernceNullable(reinterpret_cast<Nullable<double> *>(pValue), val.isNull());
            break;
        case eFieldString:
            pValue = DerefernceNullable(reinterpret_cast<Nullable<String> *>(pValue), val.isNull());
            break;
        case eFieldDateTime:
            pValue = DerefernceNullable(reinterpret_cast<Nullable<DateTime> *>(pValue), val.isNull());
            break;
        case eFieldVariant:
            pValue = DerefernceNullable(reinterpret_cast<Nullable<Variant> *>(pValue), val.isNull());
            break;
        }
    }
    if (!pValue)
        return;

    switch (type)
    {
    default:
    case eFieldVoid:
        throw std::invalid_argument(std::string("Unknown field type: ") + (char *)type);
    case eFieldBool:
        if (!val.isBool()) throw std::invalid_argument("Type mismatch: not a boolean");
        *reinterpret_cast<bool *>(pValue) = val.asBool();
        break;
    case eFieldInt:
        if (!val.isIntegral()) throw std::invalid_argument("Type mismatch: not an int");
        *reinterpret_cast<int32_t *>(pValue) = val.asInt();
        break;
    case eFieldUint:
        if (!val.isIntegral()) throw std::invalid_argument("Type mismatch: not an uint");
        *reinterpret_cast<uint32_t *>(pValue) = val.asUInt();
        break;
    case eFieldInt64:
        if (!val.isIntegral()) throw std::invalid_argument("Type mismatch: not a long");
        *reinterpret_cast<int64_t *>(pValue) = val.asInt64();
        break;
    case eFieldUint64:
        if (!val.isIntegral()) throw std::invalid_argument("Type mismatch: not an ulong");
        *reinterpret_cast<uint64_t *>(pValue) = val.asUInt64();
        break;
    case eFieldFloat:
        if (!val.isDouble()) throw std::invalid_argument("Type mismatch: not a float");
        *reinterpret_cast<float *>(pValue) = (float)val.asDouble();
        break;
    case eFieldDouble:
        if (!val.isDouble()) throw std::invalid_argument("Type mismatch: not a double");
        *reinterpret_cast<double *>(pValue) = val.asDouble();
        break;
    case eFieldString:
        if (!val.isString()) throw std::invalid_argument("Type mismatch: not a string");
        *reinterpret_cast<metacpp::String *>(pValue) = val.asString();
        break;
    case eFieldEnum:
        if (field && val.isString())
        {
            *reinterpret_cast<uint32_t *>(pValue) = reinterpret_cast<const MetaFieldEnum *>(field)->fromString(val.asCString());
            break;
        }
        if (!val.isUInt()) throw std::invalid_argument("Type mismatch: invalid enum");
        *reinterpret_cast<uint32_t *>(pValue) = val.asUInt();
        break;
    case eFieldArray: {
        if (!field)
            throw std::invalid_argument("Nested arrays are not supported");
        if (!val.isArray() && !val.isNull())  throw std::invalid_argument("Type mismatch: not an array");
        metacpp::Array<char> *arrayValue = reinterpret_cast<metacpp::Array<char> *>(pValue);
        arrayValue->resize(val.size());
        for (size_t i = 0; i < val.size(); ++i)
        {
            void *pValue = arrayValue->data() + reinterpret_cast<const MetaFieldArray *>(field)->arrayElementSize() * i;
            parseValue(val.get((Json::ArrayIndex)i, Json::Value()),
                       reinterpret_cast<const MetaFieldArray *>(field)->arrayElementType(),
                       pValue);
        }
        break;
    }
    case eFieldObject: {
        JsonDeserializerVisitor nestedSerializer(val, m_typeResolver);
        nestedSerializer.visit(reinterpret_cast<Object *>(pValue));
        break;
    }
    case eFieldDateTime:
        if (val.isNull()) {
            *reinterpret_cast<DateTime *>(pValue) = DateTime();
        }
        else {
            if (!val.isString())
                throw std::invalid_argument("Type mismatch: not a timestamp");
            *reinterpret_cast<DateTime *>(pValue) = DateTime::fromString(val.asCString());
        }
        break;
    case eFieldVariant:
        *reinterpret_cast<Variant *>(pValue) = ToVariant(val, m_typeResolver);
        break;
    } // switch
}

} // namespace json
} // namespace serialization
} // namespace metacpp
