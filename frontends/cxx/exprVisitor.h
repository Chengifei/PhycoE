/* Copyright 2017-2018 by Yifei Zheng
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXPR_VISITOR_HPP
#define EXPR_VISITOR_HPP
#include <antlr4-runtime.h>
#include "exprParser.h"
#include <support/Expr.hpp>

// We don't return anything, but it's required for antlr4
class exprVisitor {
public:
    support::Expr expr;
    int visitUnit(exprParser::UnitContext* ctx) {
        if (ctx->Identifier() || ctx->Number()) {
            std::string* data = new std::string(ctx->children[0]->getText());
            expr = support::Expr();
            expr.data = reinterpret_cast<const char*>(data);
        }
        else if (ctx->functionExpression())
            visitFunctionExpression(static_cast<exprParser::FunctionExpressionContext*>(ctx->children[0]));
        else
            visitExpression(static_cast<exprParser::ExpressionContext*>(ctx->children[1]));
    }

    int visitFunctionExpression(exprParser::FunctionExpressionContext* ctx) {
        std::string* name = new std::string(ctx->children[0]->getText());
        // minus 3 for function name, lparen, rparen, plus one for pairing up
        // the last argument with comma
        std::size_t sz = (ctx->children.size() - 3 + 1) / 2;
        support::Expr root(new support::Expr::Op(sz, name));
        auto it = ctx->children.cbegin() + 2;
        for (std::size_t i = 0; i != sz; ++i, it += 2) {
            visitExpression(static_cast<exprParser::ExpressionContext *>(*it));
            root.op->args[i] = std::move(expr);
        }
    }

    int visitPowExpression(exprParser::PowExpressionContext* ctx) {
        if (ctx->children.size() == 3) {
            std::string* data = new std::string(ctx->children[1]->getText());
            support::Expr root(new support::Expr::Op(2, data));
            visitPowExpression(static_cast<exprParser::PowExpressionContext*>(ctx->children[0]));
            root.op->args[0] = std::move(expr);
            visitUnit(static_cast<exprParser::UnitContext*>(ctx->children[2]));
            root.op->args[1] = std::move(expr);
            expr = std::move(root);
        }
        else {
            visitUnit(static_cast<exprParser::UnitContext*>(ctx->children[0]));
        }
    }

    int visitMultiplicativeExpression(exprParser::MultiplicativeExpressionContext* ctx) {
        if (ctx->children.size() == 3) {
            std::string* data = new std::string(ctx->children[1]->getText());
            support::Expr root(new support::Expr::Op(2, data));
            visitMultiplicativeExpression(static_cast<exprParser::MultiplicativeExpressionContext*>(ctx->children[0]));
            root.op->args[0] = std::move(expr);
            visitPowExpression(static_cast<exprParser::PowExpressionContext*>(ctx->children[2]));
            root.op->args[1] = std::move(expr);
            expr = std::move(root);
        }
        else {
            visitPowExpression(static_cast<exprParser::PowExpressionContext*>(ctx->children[0]));
        }
    }

    int visitExpression(exprParser::ExpressionContext* ctx) {
        if (ctx->children.size() == 3) {
            std::string* data = new std::string(ctx->children[1]->getText());
            support::Expr root(new support::Expr::Op(2, data));
            visitExpression(static_cast<exprParser::ExpressionContext*>(ctx->children[0]));
            root.op->args[0] = std::move(expr);
            visitMultiplicativeExpression(static_cast<exprParser::MultiplicativeExpressionContext*>(ctx->children[2]));
            root.op->args[1] = std::move(expr);
            expr = std::move(root);
        }
        else {
            visitMultiplicativeExpression(static_cast<exprParser::MultiplicativeExpressionContext*>(ctx->children[0]));
        }
    }
};
#endif
