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

#include <coreobjects/eval_value_lexer.h>
#include <coretypes/exceptions.h>

using TokenType = EvalValueToken::Type;


EvalValueLexer::EvalValueLexer(const std::string& input)
    : source(input)
    , index(0)
{
};

std::vector<EvalValueToken> EvalValueLexer::tokenize()
{
    scannedTokens.clear();
    while (!isAtEnd())
        scanToken();
    scannedTokens.emplace_back(EvalValueToken{TokenType::End});
    return scannedTokens;
}

void EvalValueLexer::scanToken()
{
#define EMIT_TOKEN_AND_RETURN(tokenType, advanceCount) do { emitTokenAndAdvance(tokenType, advanceCount); return; } while (0)

    char nextChar = peek();
    if (nextChar > 0)
    {
        switch (nextChar)
        {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                advance();
                return;
            case '(': EMIT_TOKEN_AND_RETURN(TokenType::OpenParen, 1);
            case ')': EMIT_TOKEN_AND_RETURN(TokenType::CloseParen, 1);
            case '[': EMIT_TOKEN_AND_RETURN(TokenType::OpenBracket, 1);
            case ']': EMIT_TOKEN_AND_RETURN(TokenType::CloseBracket, 1);
            case '{': EMIT_TOKEN_AND_RETURN(TokenType::OpenCurly, 1);
            case '}': EMIT_TOKEN_AND_RETURN(TokenType::CloseCurly, 1);
            case '+': EMIT_TOKEN_AND_RETURN(TokenType::Plus, 1);
            case '-': EMIT_TOKEN_AND_RETURN(TokenType::Minus, 1);
            case '*': EMIT_TOKEN_AND_RETURN(TokenType::Star, 1);
            case '/': EMIT_TOKEN_AND_RETURN(TokenType::Slash, 1);
            case ',': EMIT_TOKEN_AND_RETURN(TokenType::Comma, 1);
            case '$': EMIT_TOKEN_AND_RETURN(TokenType::Dollar, 1);
            case '%': EMIT_TOKEN_AND_RETURN(TokenType::Percent, 1);
            case ':': EMIT_TOKEN_AND_RETURN(TokenType::Colon, 1);
            case '|':
                if (peek(1) == '|')
                    EMIT_TOKEN_AND_RETURN(TokenType::OrOr, 2);
                else
                    break;
            case '&':
                if (peek(1) == '&')
                    EMIT_TOKEN_AND_RETURN(TokenType::AndAnd, 2);
                else
                    break;
            case '=':
                if (peek(1) == '=')
                    EMIT_TOKEN_AND_RETURN(TokenType::EqEq, 2);
                else
                    break;
            case '!':
                if (peek(1) == '=')
                    EMIT_TOKEN_AND_RETURN(TokenType::NotEq, 2);
                else
                    EMIT_TOKEN_AND_RETURN(TokenType::Exclamation, 1);
            case '<':
                if (peek(1) == '=')
                    EMIT_TOKEN_AND_RETURN(TokenType::LessEq, 2);
                else
                    EMIT_TOKEN_AND_RETURN(TokenType::Less, 1);
            case '>':
                if (peek(1) == '=')
                    EMIT_TOKEN_AND_RETURN(TokenType::GreaterEq, 2);
                else
                    EMIT_TOKEN_AND_RETURN(TokenType::Greater, 1);
            case '"':
            case '\'':
                scanString(nextChar);
                return;
            case '.':
                if (std::isdigit(peek(1)))
                {
                    scanFloat();
                    return;
                }
                else
                {
                    EMIT_TOKEN_AND_RETURN(TokenType::Dot, 1);
                }
            default:
                if (std::isdigit(nextChar))
                {
                    scanNumber();
                    return;
                }
                else if (std::isalpha(nextChar) || nextChar == '_')
                {
                    std::string name = advanceOverIdentifier();
                    std::string identifier = EvalValueToken::toCanonicalForm(name);

                    if (identifier == "if")     { emitToken(TokenType::If); return; }
                    if (identifier == "switch") { emitToken(TokenType::Switch); return; }
                    if (identifier == "true")   { emitToken(TokenType::BoolValue, true); return; }
                    if (identifier == "false")  { emitToken(TokenType::BoolValue, false); return; }
                    if (identifier == "unit")   { emitToken(TokenType::Unit); return; }

                    emitToken(TokenType::Identifier, name);
                    return;
                }
        }
#undef EMIT_TOKEN_AND_RETURN

    }
    throw daq::ParseFailedException("syntax error");
}

void EvalValueLexer::scanString(char delimiter)
{
    advance();
    if (isAtEnd())
        return;
    size_t lexemeStart = index;
    // TODO: we don't handle escaped delimiters
    while (peek() != delimiter)
    {
        advance();
        if (isAtEnd())
            return;
    }
    auto value = source.substr(lexemeStart, index - lexemeStart);
    emitToken(TokenType::StringValue, value);
    advance();
}

void EvalValueLexer::scanNumber()
{
    size_t lexemeStart = index;
    while (std::isdigit(peek()))
        advance();
    if (peek() != '.' && std::tolower(peek()) != 'e')
    {
        int64_t value = std::stoll(source.substr(lexemeStart, index - lexemeStart));
        emitToken(TokenType::IntValue, value);
    }
    else
    {
        index = lexemeStart;
        scanFloat();
    }
}

void EvalValueLexer::scanFloat()
{
    size_t lexemeStart = index;
    while (std::isdigit(peek()))
        advance();
    if (peek() == '.')
    {
        advance();
        if (!std::isdigit(peek()))
            throw daq::ParseFailedException("expected at least one digit after decimal point");
        while (std::isdigit(peek()))
            advance();
    }
    if (std::tolower(peek()) == 'e')
    {
        advance();
        if (peek() == '+' || peek() == '-')
            advance();
        if (!std::isdigit(peek()))
            throw daq::ParseFailedException("invalid exponent");
        while (std::isdigit(peek()))
            advance();
    }
    emitToken(TokenType::FloatValue, std::stod(source.substr(lexemeStart, index - lexemeStart)));
}

std::string EvalValueLexer::advanceOverIdentifier()
{
    size_t lexemeStart = index;
    while (std::isalnum(peek()) || peek() == '_' || peek() == '.')
    {
        advance();
        if (isAtEnd())
            break;
    }
    return source.substr(lexemeStart, index - lexemeStart);
}


void EvalValueLexer::emitToken(TokenType type, EvalValueToken::Value lexeme)
{
    scannedTokens.emplace_back(EvalValueToken{type, lexeme});
}

void EvalValueLexer::emitTokenAndAdvance(TokenType type, int advanceCount)
{
    emitToken(type);
    index += advanceCount;
}

bool EvalValueLexer::isAtEnd() const
{
    return index >= source.size();
}

char EvalValueLexer::peek(int n) const
{
    if (index + n >= source.size())
        return '\0';
    return source[index + n];
}

void EvalValueLexer::advance()
{
    index++;
}
