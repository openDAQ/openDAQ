grammar CoreTemplate;

start:
     ( variable
     | ifStmt
     | forStmt
     | Newline
     )*
     ;

variable: '$' '(' expression ')';

ifStmt: '%' 'if' '(' expression ')' '<%' Newline
            body
            ('%' 'else' 'if' '(' expression ')' '<%' Newline body )?
            ('%' 'else' Newline body)?
        '%>';

body: start;

forStmt: '%' 'foreach' (openFor | closedFor) '<%' Newline body '%>';

openFor: variable 'in' variable;

closedFor: '(' Identifier 'in' props ')';

expression: props  check?;

props: prop ('.' prop)*;

prop: Identifier;


check: op (expression | literal);

literal: Identifier | Number | String | Bool;

op: '<'
  | '>'
  | '<' '='
  | '>' '='
  | '=='
  | '!='
  ;

String: '"' .*? '"';

Dollar: '$';
LAngle : '<';
RAngle: '>';
Mod: '%';
LParen: '(';
RParen: ')';
EndBlock: '%>';
StartBlock: '<%';

//Anything:  (~'$')*;

Identifier
    :   IdentifierNondigit
        (   IdentifierNondigit
        |   Digit
        )*
    ;

HexadecimalConstant
    :   HexadecimalPrefix HexadecimalDigit+
    ;

Bool: 'true' | 'false';

Number: Digit+ ('.' Digit+)?;

AlphaNumeric : (Digit | Nondigit);

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
        -> skip
    ;

Comment: '#' ~[\r\n]* '\r'? '\n' -> channel(HIDDEN);

Newline
    :   (   '\r' '\n'?
        |   '\n'
        ) //-> skip
        ;