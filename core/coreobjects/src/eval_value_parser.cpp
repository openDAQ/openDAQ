/*
 * Blueberry d.o.o. ("COMPANY") CONFIDENTIAL
 * Unpublished Copyright (c) 2021-2022 Blueberry d.o.o., All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains the property of
 * COMPANY. The intellectual and technical concepts contained herein are
 * proprietary to COMPANY and are protected by copyright law and as trade
 * secrets and may also be covered by U.S. and Foreign Patents, patents in
 * process, etc.
 * Dissemination of this information or reproduction of this material is
 * strictly forbidden unless prior written permission is obtained from COMPANY.
 * Access to the source code contained herein is hereby forbidden to anyone
 * except current COMPANY employees, managers or contractors who have executed
 * Confidentiality and Non-disclosure agreements explicitly covering such
 * access.
 *
 * The copyright notice above does not evidence any actual or intended
 * publication or disclosure  of  this source code, which includes information
 * that is confidential and/or proprietary, and is a trade secret of COMPANY.
 * ANY REPRODUCTION, MODIFICATION, DISTRIBUTION, PUBLIC PERFORMANCE, OR PUBLIC
 * DISPLAY OF OR THROUGH USE OF THIS SOURCE CODE WITHOUT THE EXPRESS
 * WRITTEN CONSENT OF COMPANY IS STRICTLY PROHIBITED, AND IN VIOLATION OF
 * APPLICABLE LAWS AND INTERNATIONAL TREATIES. THE RECEIPT OR POSSESSION OF
 * THIS SOURCE CODE AND/OR RELATED INFORMATION DOES NOT CONVEY OR IMPLY ANY
 * RIGHTS TO REPRODUCE, DISCLOSE OR DISTRIBUTE ITS CONTENTS, OR TO MANUFACTURE,
 * USE, OR SELL ANYTHING THAT IT  MAY DESCRIBE, IN WHOLE OR IN PART.
 */

#include <coreobjects/eval_value_parser.h>
#include <memory>

 
using TokenType = EvalValueToken::Type;


bool parseEvalValue(const std::string& str, ParseParams* params)
{
    assert(params != nullptr);
    try
    {
        auto tokens = EvalValueLexer(str).tokenize();
        auto parser = EvalValueParser(tokens, params);
        params->node = parser.parse();
        params->propertyReferences = parser.getPropertyReferences();
        return true;
    }
    catch (const std::exception& e)
    {
        params->errMessage = e.what();
        return false;
    }
}


EvalValueParser::EvalValueParser(const std::vector<EvalValueToken>& aTokens, ParseParams* aParams)
    : tokens(aTokens)
    , current(0)
    , params(aParams)
{
    assert(!aTokens.empty() && aTokens.back().type == TokenType::End);
}

std::unique_ptr<daq::BaseNode> EvalValueParser::parse()
{
    return expression();
}

std::unique_ptr<std::unordered_set<std::string>> EvalValueParser::getPropertyReferences()
{
    return std::make_unique<std::unordered_set<std::string>>(std::move(propertyReferences));
}


std::unique_ptr<daq::BaseNode> EvalValueParser::expression()
{
    return equality();
}

std::unique_ptr<daq::BaseNode> EvalValueParser::equality()
{
    auto expr = logicOp();
    while (isAtAnyOf({TokenType::EqEq, TokenType::NotEq,
                      TokenType::Less, TokenType::LessEq,
                      TokenType::Greater, TokenType::GreaterEq}))
    {
        std::unique_ptr<daq::BinaryNode> node;
        switch (advance().type)
        {
            case TokenType::EqEq:      node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::equals>>(); break;
            case TokenType::NotEq:     node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::notEquals>>(); break;
            case TokenType::Less:      node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::less>>(); break;
            case TokenType::LessEq:    node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::lessOrEqual>>(); break;
            case TokenType::Greater:   node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::greater>>(); break;
            case TokenType::GreaterEq: node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::greaterOrEqual>>(); break;
            default: assert(false);
        }
        node->leftNode = std::move(expr);
        node->rightNode = logicOp();
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<daq::BaseNode> EvalValueParser::logicOp()
{
    auto expr = term();
    while (isAtAnyOf({TokenType::OrOr, TokenType::AndAnd}))
    {
        std::unique_ptr<daq::BinaryNode> node;
        switch (advance().type)
        {
            case TokenType::OrOr:   node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::logOr>>(); break;
            case TokenType::AndAnd: node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::logAnd>>(); break;
            default: assert(false);
        }
        node->leftNode = std::move(expr);
        node->rightNode = term();
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<daq::BaseNode> EvalValueParser::term()
{
    auto expr = factor();
    while(isAtAnyOf({TokenType::Plus, TokenType::Minus}))
    {
        std::unique_ptr<daq::BinaryNode> node;
        switch (advance().type)
        {
            case TokenType::Plus:  node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::add>>(); break;
            case TokenType::Minus: node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::sub>>(); break;
            default: assert(false);
        }
        node->leftNode = std::move(expr);
        node->rightNode = factor();
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<daq::BaseNode> EvalValueParser::factor()
{
    auto expr = unary();
    while (isAtAnyOf({TokenType::Star, TokenType::Slash}))
    {
        std::unique_ptr<daq::BinaryNode> node;
        switch (advance().type)
        {
            case TokenType::Star:  node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::mul>>(); break;
            case TokenType::Slash: node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::div>>(); break;
            default: assert(false);
        }
        node->leftNode = std::move(expr);
        node->rightNode = unary();
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<daq::BaseNode> EvalValueParser::unary()
{
    if (!isAtAnyOf({TokenType::Exclamation, TokenType::Minus}))
        return literal();

    std::unique_ptr<daq::UnaryNode> node;
    switch (advance().type)
    {
        case TokenType::Exclamation: node = std::make_unique<daq::UnaryOpNode<daq::UnaryOperationType::LogNegate>>(); break;
        case TokenType::Minus:       node = std::make_unique<daq::UnaryOpNode<daq::UnaryOperationType::Negate>>(); break;
        default: assert(false);
    }
    node->expNode = unary();
    return node;
}

std::unique_ptr<daq::BaseNode> EvalValueParser::literal()
{
    EvalValueToken token = advance();
    switch (token.type)
    {
        case TokenType::FloatValue:  return std::make_unique<daq::FloatConstNode>(std::get<double>(token.value));
        case TokenType::IntValue:    return std::make_unique<daq::IntConstNode>(std::get<int64_t>(token.value));
        case TokenType::BoolValue:   return std::make_unique<daq::BoolConstNode>(std::get<bool>(token.value));
        case TokenType::StringValue: return std::make_unique<daq::StringConstNode>(std::get<std::string>(token.value));
        case TokenType::OpenParen:
            {
                auto expr = expression();
                consume(TokenType::CloseParen);
                return expr;
            }
        case TokenType::OpenBracket:
            {
                auto elements = std::make_unique<std::vector<std::unique_ptr<daq::BaseNode>>>();
                while (!isAt(TokenType::CloseBracket))
                {
                    elements->push_back(expression());
                    if (!isAt(TokenType::CloseBracket))
                        consume(TokenType::Comma);
                }
                consume(TokenType::CloseBracket);
                return std::make_unique<daq::ListNode>(std::move(elements));
            }
        case TokenType::If:
            {
                consume(TokenType::OpenParen);
                auto ifNode = std::make_unique<daq::IfNode>();
                ifNode->condNode = expression();
                consume(TokenType::Comma);
                ifNode->trueNode = expression();
                consume(TokenType::Comma);
                ifNode->falseNode = expression();
                consume(TokenType::CloseParen);
                return ifNode;
            }
        case TokenType::Switch:
            {
                consume(TokenType::OpenParen);
                auto varNode = expression();
                auto valueNodes = std::make_unique<std::vector<std::unique_ptr<daq::BaseNode>>>();
                while (!isAt(TokenType::CloseParen))
                {
                    consume(TokenType::Comma);
                    valueNodes->push_back(expression());
                }
                consume(TokenType::CloseParen);
                if (valueNodes->size() < 2)
                    throw daq::ParseFailedException("switch statement must have at least two values");
                return std::make_unique<daq::SwitchNode>(std::move(varNode), std::move(valueNodes));
            }
        case TokenType::Unit:
        {
            consume(TokenType::OpenParen);

            auto unitParams = std::make_unique<std::vector<std::unique_ptr<daq::BaseNode>>>();
            while(!isAt(TokenType::CloseParen))
            {
                if (unitParams->size() > 3)
                    throw daq::ParseFailedException("Unit literal accepts up to 4 arguments");

                unitParams->push_back(literal());
                if (!isAt(TokenType::CloseParen))
                    consume(TokenType::Comma);
            }
            consume(TokenType::CloseParen);

            if (unitParams->empty())
                throw daq::ParseFailedException("Unit literal must have at least one argument (symbol)");

            auto unitNode = std::make_unique<daq::UnitNode>(std::move(unitParams));
            return unitNode;
        }
        case TokenType::Dollar: return valref();
        case TokenType::Percent: return propref();
        case TokenType::OpenCurly:
            {
                int64_t argNum = std::get<std::int64_t>(advance().value);
                consume(TokenType::CloseCurly);
                consume(TokenType::Dot);
                std::unique_ptr<daq::BaseNode> nextNode;
                switch (advance().type)
                {
                    case TokenType::Percent: nextNode = propref(); break;
                    case TokenType::Dollar: nextNode = valref(); break;
                    default: throw daq::ParseFailedException("unexpected token found");
                }

                auto node = std::make_unique<daq::RefNode>(argNum);
                node->onResolveReference = params->onResolveReference;
                node->useAsArgument(static_cast<daq::RefNode*>(nextNode.get()));
                return node;
            }
        case TokenType::Identifier:
            {
                if (!params->useFunctionAsReferenceResolver)
                    throw daq::ParseFailedException("invalid identifier");

                std::string str = std::get<std::string>(token.value);
                auto node = std::make_unique<daq::RefNode>(str, daq::RefType::Func);
                node->onResolveReference = params->onResolveReference;
                return node;
            }
        default:
            undoAdvance();
    }
    throw daq::ParseFailedException("syntax error");
}

std::unique_ptr<daq::BaseNode> EvalValueParser::valref()
{
    // The old parser used onResolveReference to figure
    // out the references from the entire refvar token, so we
    // reconstruct it back again over here. Probably should
    // just somehow use the fact that we already parsed the
    // refvar token into individual components and fill that
    // into daq::RefNode instead.
    assertIsAt(TokenType::Identifier);
    std::string str = std::get<std::string>(advance().value);

    if (isAt(TokenType::OpenBracket))
    {
        consume(TokenType::OpenBracket);
        str += "[" + std::to_string(std::get<int64_t>(advance().value)) + "]";
        consume(TokenType::CloseBracket);
    }

    auto node = std::make_unique<daq::RefNode>(str, daq::RefType::Value);
    node->onResolveReference = params->onResolveReference;
    return node;
}

std::unique_ptr<daq::BaseNode> EvalValueParser::propref()
{
    assertIsAt(TokenType::Identifier);
    std::string str = std::get<std::string>(advance().value);
    auto refType = daq::RefType::Property;
    if (isAt(TokenType::Colon))
    {
        consume(TokenType::Colon);
        assertIsAt(TokenType::Identifier);
        auto identifier = std::get<std::string>(advance().value);
        auto propertyItem = EvalValueToken::toCanonicalForm(identifier);
        if (propertyItem == "value")
            refType = daq::RefType::Value;
        else if (propertyItem == "selectedvalue")
            refType = daq::RefType::SelectedValue;
        else
            throw daq::ParseFailedException("syntax error");
        str += ":" + propertyItem;
    }
    if (isAt(TokenType::OpenParen))
    {
        consume(TokenType::OpenParen);
        consume(TokenType::CloseParen);
    }

    if (refType == daq::RefType::Property)
        propertyReferences.insert(str);

    auto node = std::make_unique<daq::RefNode>(str, refType);
    node->onResolveReference = params->onResolveReference;
    return node;
}


bool EvalValueParser::isAt(TokenType tokenType) const
{
    if (isAtEnd())
        return false;
    return peek().type == tokenType;
}

bool EvalValueParser::isAtEnd() const
{
    return peek().type == TokenType::End;
}

bool EvalValueParser::isAtAnyOf(std::initializer_list<TokenType> tokenTypes) const
{
    return std::find(tokenTypes.begin(), tokenTypes.end(), peek().type) != tokenTypes.end();
}

void EvalValueParser::assertIsAt(TokenType tokenType) const
{
    if (!isAt(tokenType))
        throw daq::ParseFailedException("syntax error");
}

void EvalValueParser::consume(TokenType tokenType)
{
    assertIsAt(tokenType);
    advance();
}

EvalValueToken EvalValueParser::advance()
{
    auto prevToken = peek();
    if (!isAtEnd())
        current++;
    return prevToken;
}

void EvalValueParser::undoAdvance()
{
    if (current > 0)
        current--;
}

EvalValueToken EvalValueParser::peek() const
{
    return tokens.at(current);
}
