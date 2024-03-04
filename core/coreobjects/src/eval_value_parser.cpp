/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

    registerPrefix(TokenType::FloatValue);
    registerPrefix(TokenType::IntValue);
    registerPrefix(TokenType::BoolValue);
    registerPrefix(TokenType::StringValue);
    registerPrefix(TokenType::Exclamation);
    registerPrefix(TokenType::Minus);
    registerPrefix(TokenType::OpenParen);
    registerPrefix(TokenType::OpenBracket);
    registerPrefix(TokenType::If);
    registerPrefix(TokenType::Switch);
    registerPrefix(TokenType::Unit);
    registerPrefix(TokenType::Dollar);
    registerPrefix(TokenType::Percent);
    registerPrefix(TokenType::OpenCurly);
    registerPrefix(TokenType::Identifier);

    registerInfix(TokenType::EqEq,      OperatorPrecedence::Equality);
    registerInfix(TokenType::NotEq,     OperatorPrecedence::Equality);
    registerInfix(TokenType::Less,      OperatorPrecedence::Equality);
    registerInfix(TokenType::LessEq,    OperatorPrecedence::Equality);
    registerInfix(TokenType::Greater,   OperatorPrecedence::Equality);
    registerInfix(TokenType::GreaterEq, OperatorPrecedence::Equality);
    registerInfix(TokenType::AndAnd,    OperatorPrecedence::Logic);
    registerInfix(TokenType::OrOr,      OperatorPrecedence::Logic);
    registerInfix(TokenType::Plus,      OperatorPrecedence::Term);
    registerInfix(TokenType::Minus,     OperatorPrecedence::Term);
    registerInfix(TokenType::Star,      OperatorPrecedence::Factor);
    registerInfix(TokenType::Slash,     OperatorPrecedence::Factor);
}

std::unique_ptr<daq::BaseNode> EvalValueParser::parse()
{
    return expression();
}

std::unique_ptr<std::unordered_set<std::string>> EvalValueParser::getPropertyReferences()
{
    return std::make_unique<std::unordered_set<std::string>>(std::move(propertyReferences));
}

std::unique_ptr<daq::BaseNode> EvalValueParser::expression(int precedence)
{
    auto token = advance();
    if (token.type == TokenType::End || prefixParseRules.find(token.type) == prefixParseRules.end())
        throw daq::ParseFailedException("Unexpected end of expression");
    auto left = prefix(token, prefixParseRules[token.type]);
    while (precedence < infixTokenPrecedence(peek().type))
    {
        token = advance();
        left = infix(token, std::move(left), infixParseRules[token.type]);
    }
    return left;
}

std::unique_ptr<daq::BaseNode> EvalValueParser::infix(const EvalValueToken& token, std::unique_ptr<daq::BaseNode> left, const ParseRule& rule)
{
    std::unique_ptr<daq::BinaryNode> node;
    switch (token.type)
    {
        case TokenType::EqEq:      node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::equals>>(); break;
        case TokenType::NotEq:     node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::notEquals>>(); break;
        case TokenType::Less:      node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::less>>(); break;
        case TokenType::LessEq:    node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::lessOrEqual>>(); break;
        case TokenType::Greater:   node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::greater>>(); break;
        case TokenType::GreaterEq: node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::greaterOrEqual>>(); break;
        case TokenType::OrOr:      node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::logOr>>(); break;
        case TokenType::AndAnd:    node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::logAnd>>(); break;
        case TokenType::Plus:      node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::add>>(); break;
        case TokenType::Minus:     node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::sub>>(); break;
        case TokenType::Star:      node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::mul>>(); break;
        case TokenType::Slash:     node = std::make_unique<daq::BinaryOpNode<daq::BinOperationType::div>>(); break;
        default: assert(false);
    }
    node->leftNode = std::move(left);
    node->rightNode = expression(rule.precedence - (rule.associativity == Associativity::Right ? 1 : 0));
    return node;
}

std::unique_ptr<daq::BaseNode> EvalValueParser::prefix(const EvalValueToken& token, const ParseRule& rule)
{
    switch (token.type)
    {
        case TokenType::FloatValue:  return std::make_unique<daq::FloatConstNode>(std::get<double>(token.value));
        case TokenType::IntValue:    return std::make_unique<daq::IntConstNode>(std::get<int64_t>(token.value));
        case TokenType::BoolValue:   return std::make_unique<daq::BoolConstNode>(std::get<bool>(token.value));
        case TokenType::StringValue: return std::make_unique<daq::StringConstNode>(std::get<std::string>(token.value));
        case TokenType::Exclamation:
            {
                auto node = std::make_unique<daq::UnaryOpNode<daq::UnaryOperationType::LogNegate>>();
                node->expNode = expression(rule.precedence);
                return node;
            }
        case TokenType::Minus:
            {
                auto node = std::make_unique<daq::UnaryOpNode<daq::UnaryOperationType::Negate>>();
                node->expNode = expression(rule.precedence);
                return node;
            }
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

                    unitParams->push_back(expression());
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
            throw daq::ParseFailedException("syntax error");
    }
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


void EvalValueParser::registerPrefix(TokenType type, int precedence)
{
    prefixParseRules[type] = {precedence};
}

void EvalValueParser::registerInfix(TokenType type, int precedence, Associativity associativity)
{
    infixParseRules[type] = {precedence, associativity};
}


int EvalValueParser::infixTokenPrecedence(EvalValueToken::Type tokenType) const
{
    if (tokenType == TokenType::End || infixParseRules.find(tokenType) == infixParseRules.end())
        return OperatorPrecedence::MinPrecedence;
    return infixParseRules.at(tokenType).precedence;
}

bool EvalValueParser::isAt(TokenType tokenType) const
{
    return peek().type == tokenType;
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
    if (prevToken.type != TokenType::End)
        current++;
    return prevToken;
}

EvalValueToken EvalValueParser::peek() const
{
    return tokens.at(current);
}
