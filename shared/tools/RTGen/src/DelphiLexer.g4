lexer grammar DelphiLexer;

channels {
    WHITESPACE_CHANNEL,
    COMMENTS_CHANNEL
}

/*
 * Named tokens
 */
String: '\'' .*? '\'';

Dot: '.';

Unit: 'unit';
TypeBlock: 'type';

AccessModifier: 'public' | 'private' | 'protected';

Interface: 'interface';
Implementation: 'implementation';
Uses: 'uses';
Function: 'function';
Procedure: 'procedure';

End: 'end' ;
Out: 'out';
Var: 'var';

HashTag: '#';
Quote: '"';
QuoteSingle: '\'';
Hyphen: '-';
LessThan : '<';
GreaterThan : '>';
Caret: '^';
LParen : '(';
RParen: ')';
LBrace: '{';
RBrace: '}';
LBracket: '[';
RBracket: ']';
Comma: ',';
Slash: '/';
Semicolon: ';';
AssignEquals: '=';
Static: 'static';
Const: 'const';
TypeDef: 'typedef';
Struct: 'struct';
Class: 'class';
Public: 'public';
Private: 'private';
Protected: 'protected';
Virtual: 'virtual';
Enum: 'enum';
Ampersand: '&';
Star: '*';
Colon: ':';
DoubleColon: '::';
Exclamation: '!';
CommentStart: '//';
RtCommentBlock: '/*#';
BlockCommentStart : '/*';
BlockCommentEnd : '*/';

/*
 * LEXER
 */

PrimitiveValue: (Boolean | Int | String) ;

Pipe: '|';

CallingConvention: ('cdecl' | 'stdcall' | 'fastcall' | 'overload');

VarModifiers: 'static' | 'const';


Identifier
    :   IdentifierNondigit
        (   IdentifierNondigit
        |   Digit
        )*
    ;

HexadecimalConstant
    :   HexadecimalPrefix HexadecimalDigit+
    ;

Number: Digit;

Int: Digit+;

AlphaNumeric : (Digit | Nondigit);

Boolean: 'True' | 'False' ;

fragment
IdentifierNondigit
    :   Nondigit
    |   UniversalCharacterName
    //|   // other implementation-defined characters...
    ;

fragment
Nondigit
    :   [a-zA-Z_]
    ;

fragment
DigitAndCapital: [0-9A-Z] ;

fragment
Digit
    :   [0-9]
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
Letter : [a-zA-Z\u0080-\u00FF_];

Whitespace
    :   [ \t]+
        -> channel(WHITESPACE_CHANNEL)
    ;

// CommentRT: '//' '[';

Comment: '//' ~[[\r\n]* '\r'? '\n' -> channel(COMMENTS_CHANNEL);

BlockComment : '{' ~[#]*? '}' -> channel(COMMENTS_CHANNEL);

Newline
    :   (   '\r' '\n'?
        |   '\n'
        )
        -> channel(WHITESPACE_CHANNEL)
    ;

Implementations: Implementation .*? End Dot ;

StringDouble: '"' .*? '"';
