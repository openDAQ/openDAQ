/*
 * Copyright 2022-2024 openDAQ d. o. o.
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

#pragma once
#include <cctype>
#include <string>
#include <variant>
#include <vector>
#include <cstdint>

struct EvalValueToken
{
    using Value = std::variant<std::monostate, std::string, int64_t, double, bool>;

    enum class Type {
        End          = 0,

        OpenParen    = 1,
        CloseParen   = 2,
        OpenBracket  = 3,
        CloseBracket = 4,
        OpenCurly    = 5,
        CloseCurly   = 6,

        Plus         = 100,
        Minus        = 101,
        Star         = 102,
        Slash        = 103,
        Exclamation  = 104,
        Comma        = 105,
        Dot          = 106,
        Dollar       = 107,
        Percent      = 108,
        Colon        = 109,

        OrOr         = 200,
        AndAnd       = 201,
        EqEq         = 202,
        NotEq        = 203,
        Less         = 204,
        LessEq       = 205,
        Greater      = 206,
        GreaterEq    = 207,

        IntValue     = 300,
        BoolValue    = 301,
        FloatValue   = 302,
        StringValue  = 303,
        Identifier   = 304,
        Unit         = 305,

        Switch       = 400,
        If           = 401,
    } type;
    Value value;

    static std::string toCanonicalForm(const std::string& identifier)
    {
        std::string canonicalForm = identifier;
        for (size_t i = 0; i < canonicalForm.size(); ++i)
            canonicalForm[i] = std::tolower(canonicalForm[i]);
        return canonicalForm;
    }
};

class EvalValueLexer
{
public:
    EvalValueLexer(const std::string& input);

    std::vector<EvalValueToken> tokenize();

private:
    void scanToken();
    void scanString(char delimiter);
    void scanNumber();
    void scanFloat();
    std::string advanceOverIdentifier();

    void emitToken(EvalValueToken::Type type, EvalValueToken::Value lexeme = {});
    bool isAtEnd() const;
    char peek(int n = 0) const;
    void advance();

    void emitTokenAndAdvance(EvalValueToken::Type type, int advanceCount = 1);

    std::string source;
    size_t index;
    std::vector<EvalValueToken> scannedTokens;
};
