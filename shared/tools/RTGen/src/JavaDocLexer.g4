lexer grammar JavaDocLexer;

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

fragment
Emoji: [\u{1F4A9}\u{1F926}]
     ;

fragment
OtherPunctuation: [#.,?!&%"'/\\]
                ;

fragment
UnicodePunctuation: [\p{Pc}\p{Pd}\p{Ps}\p{Pe}\p{Pi}\p{Pf};:]
                  | OtherPunctuation
                  ;

fragment
DocChar: ( UnicodeIdentifier
         | UnicodePunctuation
         | [\p{Symbol}]
         | Emoji
         )+;

AtSign: '@';
LBrace: '{';
RBrace: '}';
LBracket: '[';
RBracket: ']';

DocStartSection: '@{';
DocEndSection: '@}';
//
DocCode: AtSign 'code';
DocEndCode: AtSign 'endcode';
DocStartCode: DocCode .*? DocEndCode;
//
DocPrivate: AtSign 'private';
DocRetVal: AtSign 'retval';
DocParam: AtSign 'param' (LBracket DocWord RBracket)?;
DocBrief: AtSign 'brief';
DocThrows: AtSign 'throw' 's'?;
DocParamRef: AtSign 'p';
DocCodeRef: AtSign 'c';
DocAttribute: AtSign DocWord (LBracket DocWord RBracket)?;

DocWord: DocChar+;

WS
    :   [ \t]+
        -> channel(HIDDEN)
    ;

NL
    : (   '\r' '\n'
      |   '\n'
      )
    ;

Star: '*';
DocStart: '/*!';
DocEnd: '*/';
