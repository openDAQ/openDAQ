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

    registerPrefix(TokenType::FloatValue,  &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::IntValue,    &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::BoolValue,   &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::StringValue, &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::Exclamation, &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::Minus,       &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::OpenParen,   &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::OpenBracket, &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::If,          &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::Switch,      &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::Unit,        &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::Dollar,      &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::Percent,     &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::OpenCurly,   &EvalValueParser::parsePrefix);
    registerPrefix(TokenType::Identifier,  &EvalValueParser::parsePrefix);

    registerInfix(TokenType::EqEq,      &EvalValueParser::parseInfix, OperatorPrecedence::Equality);
    registerInfix(TokenType::NotEq,     &EvalValueParser::parseInfix, OperatorPrecedence::Equality);
    registerInfix(TokenType::Less,      &EvalValueParser::parseInfix, OperatorPrecedence::Equality);
    registerInfix(TokenType::LessEq,    &EvalValueParser::parseInfix, OperatorPrecedence::Equality);
    registerInfix(TokenType::Greater,   &EvalValueParser::parseInfix, OperatorPrecedence::Equality);
    registerInfix(TokenType::GreaterEq, &EvalValueParser::parseInfix, OperatorPrecedence::Equality);
    registerInfix(TokenType::AndAnd,    &EvalValueParser::parseInfix, OperatorPrecedence::Logic);
    registerInfix(TokenType::OrOr,      &EvalValueParser::parseInfix, OperatorPrecedence::Logic);
    registerInfix(TokenType::Plus,      &EvalValueParser::parseInfix, OperatorPrecedence::Term);
    registerInfix(TokenType::Minus,     &EvalValueParser::parseInfix, OperatorPrecedence::Term);
    registerInfix(TokenType::Star,      &EvalValueParser::parseInfix, OperatorPrecedence::Factor);
    registerInfix(TokenType::Slash,     &EvalValueParser::parseInfix, OperatorPrecedence::Factor);
}

std::unique_ptr<daq::BaseNode> EvalValueParser::parse()
{
    return parseExpression(OperatorPrecedence::MinPrecedence);
}

std::unique_ptr<daq::BaseNode> EvalValueParser::parseExpression(int precedence)
{
    auto token = advance();
    if (token.type == TokenType::End || prefixParselets.find(token.type) == prefixParselets.end())
        throw daq::ParseFailedException("Unexpected end of expression");
    auto prefixParselet = prefixParselets[token.type];
    auto left = (this->*prefixParselet.parse)(token, prefixParselet.precedence);
    while (precedence < nextTokenPrecedence())
    {
        token = advance();
        if (infixParselets.find(token.type) == infixParselets.end())
            throw daq::ParseFailedException("Unexpected token");
        auto infixParselet = infixParselets[token.type];
        left = (this->*infixParselet.parse)(token, std::move(left), infixParselet.associativity, infixParselet.precedence);
    }
    return left;
}

std::unique_ptr<std::unordered_set<std::string>> EvalValueParser::getPropertyReferences()
{
    return std::make_unique<std::unordered_set<std::string>>(std::move(propertyReferences));
}

std::unique_ptr<daq::BaseNode> EvalValueParser::parseInfix(EvalValueToken token, std::unique_ptr<daq::BaseNode> left, Associativity associativity, int parseletPrecedence)
{
    auto right = parseExpression(parseletPrecedence - (associativity == Associativity::Right ? 1 : 0));
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
    node->rightNode = std::move(right);
    return node;
}

std::unique_ptr<daq::BaseNode> EvalValueParser::parsePrefix(EvalValueToken token, int parseletPrecedence)
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
                node->expNode = parseExpression(parseletPrecedence);
                return node;
            }
        case TokenType::Minus:
            {
                auto node = std::make_unique<daq::UnaryOpNode<daq::UnaryOperationType::Negate>>();
                node->expNode = parseExpression(parseletPrecedence);
                return node;
            }
        case TokenType::OpenParen:
            {
                auto expr = parseExpression();
                consume(TokenType::CloseParen);
                return expr;
            }
        case TokenType::OpenBracket:
            {
                auto elements = std::make_unique<std::vector<std::unique_ptr<daq::BaseNode>>>();
                while (!isAt(TokenType::CloseBracket))
                {
                    elements->push_back(parseExpression());
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
                ifNode->condNode = parseExpression();
                consume(TokenType::Comma);
                ifNode->trueNode = parseExpression();
                consume(TokenType::Comma);
                ifNode->falseNode = parseExpression();
                consume(TokenType::CloseParen);
                return ifNode;
            }
        case TokenType::Switch:
            {
                consume(TokenType::OpenParen);
                auto varNode = parseExpression();
                auto valueNodes = std::make_unique<std::vector<std::unique_ptr<daq::BaseNode>>>();
                while (!isAt(TokenType::CloseParen))
                {
                    consume(TokenType::Comma);
                    valueNodes->push_back(parseExpression());
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

                    unitParams->push_back(parseExpression());
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


void EvalValueParser::registerPrefix(TokenType type, PrefixParselet parselet, int precedence)
{
    prefixParselets[type] = {parselet, precedence};
}

void EvalValueParser::registerInfix(TokenType type, InfixParselet parselet, int precedence, Associativity associativity)
{
    infixParselets[type] = {parselet, precedence, associativity};
}


int EvalValueParser::nextTokenPrecedence() const
{
    auto token = peek();
    if (token.type == TokenType::End || infixParselets.find(token.type) == infixParselets.end())
        return OperatorPrecedence::MinPrecedence;
    return infixParselets.at(token.type).precedence;
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

EvalValueToken EvalValueParser::peek() const
{
    return tokens.at(current);
}
