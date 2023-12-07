lexer grammar RTGen3Lexer;

channels {
    WHITESPACE_CHANNEL,
    COMMENTS_CHANNEL
}

/*
 * Named tokens
 */

fragment
Protected : 'protected' ;

fragment
Private : 'private' ;

fragment
Public : 'public' ;

AccessModifier: Public | Private | Protected;

RtInterface: 'DSRT_INTERFACE' | 'BB_INTERFACE' | 'OPENDAQ_INTERFACE';
DeclareRtIntf: 'DECLARE_' 'TEMPLATED_'? ('BB' | 'RT' | 'OPENDAQ') ('_' Identifier)? '_INTERFACE' ('_' Identifier)? ;
InheritsRtIntf: 'INHERITS_INTERFACE';

Pragma: 'pragma';
Include: 'include';
Define: 'define';
IfDef: 'ifdef';
EndIf: 'endif';
NullPtr: 'nullptr';
HashTag: '#';
AtSign: '@';
Quote: '"';
LessThan : '<';
GreaterThan : '>';
LParen : '(';
RParen: ')';
LBrace: '{';
RBrace: '}';
LBracket: '[';
RBracket: ']';
Comma: ',';
BackSlash: '\\';
Semicolon: ';';
AssignEquals: '=';
Using: 'using';
Static: 'static';
Constexpr: 'constexpr';
Template: 'template';
TypeName: 'typename';
Const: 'const';
NoExcept: 'noexcept';
TypeDef: 'typedef';
Struct: 'struct';
Class: 'class';
Virtual: 'virtual';
Enum: 'enum';
Extern: 'extern';
Friend: 'friend';
StaticAssert: 'static_assert';
Operator: 'operator' -> pushMode(methodImpl);
Inline: 'inline' -> pushMode(methodImpl);
Ampersand: '&';
Colon: ':';
DoubleColon: '::';
DocCommentBlock: '/*!' '<'? -> pushMode(jsDoc);
CommentStart: '//';
RtCommentBlock: '/*#';
BlockCommentStart : '/*';
BlockCommentEnd : '*/';
Exclamation: '!';
Dot: '.';
Pipe: '|';
Slash: '/';
Star: '*';

/*
 * LEXER
 */

CallingConvention: InterfaceFuncCall
                 | '__' ('cdecl' | 'stdcall' | 'fastcall');

fragment
InterfaceFuncCall: 'INTERFACE_FUNC';

VarModifiers: Static | Const;

Int: ('+' | '-')? Digit+;

Bool : 'true'
     | 'false'
     ;

MacroIdentifier
    :   NondigitUpper
        (   NondigitUpper
        |   Digit
        )+
    ;

Identifier
    :   IdentifierNondigit
        (   IdentifierNondigit
        |   Digit
        )*
    ;

BinaryConstant
    : BinaryPrefix BinaryDigit+
    ;

HexadecimalConstant
    : HexadecimalPrefix HexadecimalDigit+
    ;

AlphaNumeric : (Digit | Nondigit);


fragment
AnyIdentifier
    : Identifier
    | MacroIdentifier
    ;

String: '"' .*? '"';

fragment
IdentifierNondigit
    :   Nondigit
    |   UniversalCharacterName
    ;

fragment
Nondigit
    :   [a-zA-Z_]
    ;

fragment
NondigitUpper
    :   [A-Z_]
    ;

fragment
Digit
    : [0-9]
    ;

fragment
UniversalCharacterName
    :   '\\u' HexQuad
    |   '\\U' HexQuad HexQuad
    ;

fragment
HexadecimalDigitSequence
    :   HexadecimalDigit+
    ;

fragment
BinaryDigit
    : [01]
    ;

fragment
BinaryPrefix
    : '0' [bB]
    ;

fragment
HexadecimalDigit
    :   [0-9a-fA-F]
    ;

fragment
HexadecimalPrefix
    :   '0' [xX]
    ;

fragment
HexQuad
    :   HexadecimalDigit HexadecimalDigit (HexadecimalDigit HexadecimalDigit)?
    ;

fragment
UnicodeIdentifier
   : UnicodeStartChar
   | '0'..'9'
   | '_'
   | '\u00B7'
   | '\u0300'..'\u036F'
   | '\u203F'..'\u2040'
   ;

//fragment
//UnicodeIdentifier
//   : [\p{Letter}]
//   | [\p{Mark}]
//   | [\p{Number}]
//   | [\p{Punctuation}]
//   | [\p{Symbol}]
//   ;

fragment
UnicodeStartChar
   : 'A'..'Z' | 'a'..'z'
   | '\u00C0'..'\u00D6'
   | '\u00D8'..'\u00F6'
   | '\u00F8'..'\u02FF'
   | '\u0370'..'\u037D'
   | '\u037F'..'\u1FFF'
   | '\u200C'..'\u200D'
   | '\u2070'..'\u218F'
   | '\u2C00'..'\u2FEF'
   | '\u3001'..'\uD7FF'
   | '\uF900'..'\uFDCF'
   | '\uFDF0'..'\uFFFD'
   ;

Whitespace
    :   [ \t]+
        -> channel(WHITESPACE_CHANNEL)
    ;

Comment: '//' ~[[\r\n]* '\r'? '\n' -> channel(COMMENTS_CHANNEL);

BlockComment : '/*' ~[#!]*? '*/' -> channel(COMMENTS_CHANNEL);

Newline
    :   (   '\r' '\n'?
        |   '\n'
        )
        -> channel(WHITESPACE_CHANNEL)
    ;

IncludePath: Whitespace LessThan IncludeHPath GreaterThan;

fragment
IncludeHPath: (Dot* PathHSeparator)? AnyIdentifier (PathHSeparator AnyIdentifier)* (Dot AnyIdentifier)?;

fragment
PathHSeparator: Slash | BackSlash;

mode jsDoc;

DocBody: .*? BlockCommentEnd -> popMode;

DocWhitespace
    :   [ \t]+
        -> channel(WHITESPACE_CHANNEL)
        , type(Whitespace)
    ;
DocNL
    :   (   '\r' '\n'?
        |   '\n'
        )
        -> channel(WHITESPACE_CHANNEL)
        , type(Newline)
    ;

mode methodImpl;
OpEq: AssignEquals -> type(AssignEquals);
OpExclamation: Exclamation -> type(Exclamation);
OpComma: Comma -> type(Comma);
OpAmpersand: Ampersand -> type(Ampersand);
OpStar: Star -> type(Star);
OpInt: Int -> type(Int);
OpHexConstant: HexadecimalConstant -> type(HexadecimalConstant);
OpBinConstatn: BinaryConstant -> type(BinaryConstant);
OpBool: Bool -> type(Bool);
OpString: String -> type(String);
OpConst: Const -> type(Const);
OpNoExcept: NoExcept -> type(NoExcept);

LPar: LParen -> type(LParen);
RPar: RParen -> type(RParen);

OpLBrace: LBrace -> type(LBrace);
OpRBrace: RBrace -> type(RBrace);

OpIdentifier: Identifier -> type(Identifier);

MethodBody: LBrace ( MethodBody | ~[{}] )* RBrace  -> popMode;
OpSemicolon: Semicolon -> type(Semicolon), popMode;

OpWhitespace
    :   [ \t]+
        -> channel(WHITESPACE_CHANNEL)
        , type(Whitespace)
    ;
OpNL
    :   (   '\r' '\n'?
        |   '\n'
        )
        -> channel(WHITESPACE_CHANNEL)
        , type(Newline)
    ;