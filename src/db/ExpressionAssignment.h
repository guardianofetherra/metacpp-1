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
#ifndef METACPP_DB_EXPRESSIONASSIGNMENT_H
#define METACPP_DB_EXPRESSIONASSIGNMENT_H
#include "config.h"
#include "StringBase.h"
#include <memory>

namespace metacpp {
namespace db {

template<typename T>
class ExpressionNode;

template<typename TObj, typename TField>
class ExpressionNodeColumn;

namespace detail
{
class ExpressionNodeImplBase;
}

/** \brief Base template class representing assignement expressions */
template<typename TObj>
class ExpressionAssignmentBase
{
protected:
    ExpressionAssignmentBase() { }
public:
    virtual ~ExpressionAssignmentBase() { }

    /** \brief Gets private implementation of the left-hand side expression */
    virtual std::shared_ptr<detail::ExpressionNodeImplBase> lhs() const = 0;
    /** \brief Gets private implementation of the right-hand side expression */
    virtual std::shared_ptr<detail::ExpressionNodeImplBase> rhs() const = 0;
};

/** \brief General implementation for assignments */
template<typename TObj, typename TField1, typename TField2>
class ExpressionAssignment : public ExpressionAssignmentBase<TObj>
{
public:
    /** \brief Constructs a new instance of SqlColumnAssignment with given left hand side and right hand side */
    ExpressionAssignment(const ExpressionNodeColumn<TObj, TField1>& lhs, const ExpressionNode<TField2>& rhs)
        : m_lhs(lhs.impl()), m_rhs(rhs.impl())
    {

    }

    ~ExpressionAssignment()
    {
    }

    /** \brief Gets private implementation of the left-hand side expression */
    std::shared_ptr<detail::ExpressionNodeImplBase> lhs() const { return m_lhs; }
    /** \brief Gets private implementation of the right-hand side expression */
    std::shared_ptr<detail::ExpressionNodeImplBase> rhs() const { return m_rhs; }
private:
    std::shared_ptr<detail::ExpressionNodeImplBase> m_lhs, m_rhs;
};

} // namespace db
} // namespace metacpp

#endif // METACPP_DB_EXPRESSIONASSIGNMENT_H
