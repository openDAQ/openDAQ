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

public:
    EvalValueParser(const std::vector<EvalValueToken>& aTokens, ParseParams* aParams);

    // EvalValueParser is a recursive descent parser that parses the following
    // grammar:
    //
    // expression : equality
    // equality   : logic_op ( ( "==" | "!=" | "<" | "<=" | ">" | ">=" ) logic_op )*
    // logic_op   : term ( ( "||" | "&&" ) term )*
    // term       : factor ( ( "+" | "-" ) factor )*
    // factor     : unary ( ( "*" | "/" ) unary )*
    // unary      : ( "!" | "-" ) unary
    //            | literal
    // literal    : FLOATVALUE
    //            | INTVALUE
    //            | BOOLVALUE
    //            | STRINGVALUE
    //            | "[" ( expression ( "," expression )* )? "]"
    //            | "(" expression ")"
    //            | IF "(" expression "," expression "," expression ")"
    //            | SWITCH "(" expression ("," expression)* ")"
    //            | "$" valref
    //            | "%" propref
    //            | "{" INTVALUE "}" "." ( "$" valref | "%" propref )
    //            | IDENTIFIER
    //            | UNIT "(" literal ( "," literal)? ( "," literal)? ( "," literal)? ")"
    // valref     : IDENTIFIER ( "[" INTVALUE "]" )?
    // propref    : IDENTIFIER ( ":" PROPERTYITEM )? ( "(" ")" )?

    std::unique_ptr<daq::BaseNode> parse();

    // OPENDAQ_TODO: Modify to get only property references that the eval value can resolve to
    std::unique_ptr<std::unordered_set<std::string>> getPropertyReferences();

private:
    std::unique_ptr<daq::BaseNode> expression();
    std::unique_ptr<daq::BaseNode> equality();
    std::unique_ptr<daq::BaseNode> logicOp();
    std::unique_ptr<daq::BaseNode> term();
    std::unique_ptr<daq::BaseNode> factor();
    std::unique_ptr<daq::BaseNode> unary();
    std::unique_ptr<daq::BaseNode> literal();
    std::unique_ptr<daq::BaseNode> valref();
    std::unique_ptr<daq::BaseNode> propref();

    bool isAt(EvalValueToken::Type tokenType) const;
    bool isAtEnd() const;
    bool isAtAnyOf(std::initializer_list<EvalValueToken::Type> tokenTypes) const;
    void assertIsAt(EvalValueToken::Type tokenType) const;
    void consume(EvalValueToken::Type tokenType);
    EvalValueToken advance();
    void undoAdvance();
    EvalValueToken peek() const;

    std::vector<EvalValueToken> tokens;
    std::unordered_set<std::string> propertyReferences;
    size_t current;
    ParseParams* params;
};
