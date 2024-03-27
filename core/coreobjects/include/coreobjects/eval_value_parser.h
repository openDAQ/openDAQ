/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <coreobjects/eval_nodes.h>
#include <coreobjects/eval_value_lexer.h>
#include <unordered_set>

typedef struct
{
    std::unique_ptr<daq::BaseNode> node;
    std::unique_ptr<std::unordered_set<std::string>> propertyReferences;
    bool useFunctionAsReferenceResolver;
    daq::GetReferenceEvent onResolveReference;
    std::string errMessage;
} ParseParams;

bool parseEvalValue(const std::string& str, ParseParams* params);


class EvalValueParser
{
    enum OperatorPrecedence
    {
        MinPrecedence = 0,
        Logic,
        Equality,
        Term,
        Factor,
        Prefix,
    };

    enum class Associativity
    {
        Left,
        Right,
    };

    struct ParseRule
    {
        int precedence;
        Associativity associativity;
    };

public:
    EvalValueParser(const std::vector<EvalValueToken>& aTokens, ParseParams* aParams);

    // [precedences]
    //   logic:    "||" | "&&"
    //   equality: "==" | "!=" | "<" | "<=" | ">" | ">="
    //   term:     "+" | "-"
    //   factor:   "*" | "/"
    //   prefix:   "!" | "-" (unary)
    //
    // [grammar]
    // parse      : expression
    // expression : infix
    // infix      : prefix ( ("||" | "&&" | "==" | "!=" | "<" | "<=" | ">" | ">=" | "+" | "-" | "*" | "/" ) prefix )*
    // prefix     : FLOATVALUE
    //            | INTVALUE
    //            | BOOLVALUE
    //            | STRINGVALUE
    //            | "!" expression
    //            | "-" expression
    //            | "[" ( expression ( "," expression )* )? "]"
    //            | "(" expression ")"
    //            | IF "(" expression "," expression "," expression ")"
    //            | SWITCH "(" expression ("," expression)* ")"
    //            | "$" valref
    //            | "%" propref
    //            | "{" INTVALUE "}" "." ( "$" valref | "%" propref )
    //            | IDENTIFIER
    //            | UNIT "(" expression ( "," expression)? ( "," expression)? ( "," expression)? ")"
    // valref     : IDENTIFIER ( "[" INTVALUE "]" )?
    // propref    : IDENTIFIER ( ":" PROPERTYITEM )? ( "(" ")" )?

    std::unique_ptr<daq::BaseNode> parse();

    // OPENDAQ_TODO: Modify to get only property references that the eval value can resolve to
    std::unique_ptr<std::unordered_set<std::string>> getPropertyReferences();

private:
    std::unique_ptr<daq::BaseNode> expression(int precedence = OperatorPrecedence::MinPrecedence);
    std::unique_ptr<daq::BaseNode> infix(const EvalValueToken& token, std::unique_ptr<daq::BaseNode> left, const ParseRule& rule);
    std::unique_ptr<daq::BaseNode> prefix(const EvalValueToken& token, const ParseRule& rule);
    std::unique_ptr<daq::BaseNode> valref();
    std::unique_ptr<daq::BaseNode> propref();

    void registerInfix(EvalValueToken::Type tokenType, int precedence, Associativity associativity = Associativity::Left);
    void registerPrefix(EvalValueToken::Type tokenType, int precedence = OperatorPrecedence::Prefix);

    int infixTokenPrecedence(EvalValueToken::Type tokenType) const;
    bool isAt(EvalValueToken::Type tokenType) const;
    void assertIsAt(EvalValueToken::Type tokenType) const;
    void consume(EvalValueToken::Type tokenType);
    EvalValueToken advance();
    EvalValueToken peek() const;

    std::unordered_map<EvalValueToken::Type, ParseRule> infixParseRules;
    std::unordered_map<EvalValueToken::Type, ParseRule> prefixParseRules;

    std::vector<EvalValueToken> tokens;
    std::unordered_set<std::string> propertyReferences;
    size_t current;
    ParseParams* params;
};
