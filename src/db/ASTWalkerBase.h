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
#ifndef METACPP_DB_ASTWALKERBASE_H
#define METACPP_DB_ASTWALKERBASE_H
#include "config.h"
#include "ExpressionNode.h"

namespace metacpp {
namespace db {
namespace detail {

class ASTWalkerBase
{
public:
    explicit ASTWalkerBase(const detail::ExpressionNodeImplPtr& rootNode);

    virtual ~ASTWalkerBase();

    void doWalk();
protected:
    void visitNode(detail::ExpressionNodeImplPtr node);
    virtual void visitColumn(std::shared_ptr<detail::ExpressionNodeImplColumn> column) = 0;
    virtual void visitLiteral(std::shared_ptr<detail::ExpressionNodeImplLiteral> literal) = 0;
    virtual void visitNull(std::shared_ptr<detail::ExpressionNodeImplNull> null) = 0;
    virtual void visitUnaryOperator(std::shared_ptr<detail::ExpressionNodeImplUnaryOperator> unary) = 0;
    virtual void visitCastOperator(std::shared_ptr<detail::ExpressionNodeImplCastOperator> cast) = 0;
    virtual void visitBinaryOperator(std::shared_ptr<detail::ExpressionNodeImplBinaryOperator> binary) = 0;
    virtual void visitFunctionCall(std::shared_ptr<detail::ExpressionNodeImplFunctionCall> functionCall) = 0;
    virtual void visitWhereClauseRelational(std::shared_ptr<detail::ExpressionNodeImplWhereClauseRelational> whereClauseRelational) = 0;
    virtual void visitWhereClauseLogical(std::shared_ptr<detail::ExpressionNodeImplWhereClauseLogical> whereClauseLogical) = 0;
    virtual void visitWhereClauseConditional(std::shared_ptr<detail::ExpressionNodeImplWhereClauseConditional> whereClauseComplex) = 0;
private:
    detail::ExpressionNodeImplPtr m_rootNode;

};

} // namespace detail
} // namespace db
} // namespace metacpp

#endif // METACPP_DB_ASTWALKERBASE_H
