parser grammar RTGen3;

options {
    tokenVocab = RTGen3Lexer;
}

start :
      ( static_assert
      | usingDecl
      | typeDecl
      | preprocesor
      | macro
      | globalFunc
      | varDecl
     // | Colon
      | rtComment
      | docComment
      )*
      EOF
      ;

// JavaDoc

docComment: DocCommentBlock DocBody;

// C++

static_assert: StaticAssert LParen arguments? RParen Semicolon;

globalFunc: globalFuncModifiers? type CallingConvention? Identifier LParen arguments? RParen Const? funcBody;

funcBody: pureFuncDecl? Semicolon
        | MethodBody
        ;

extern: Extern String;

globalFuncModifiers: extern
              | Static
              | Inline
              | Constexpr
              ;

preprocesor: HashTag
           ( pragma
           | include
           | define
           | preprocesorIfDef
           | EndIf
           );

define: Define macro;

preprocesorIfDef: IfDef macro;

varDecl: varModifiers type (Identifier assignment? Semicolon)?;

varModifiers: varModifier*;

varModifier: Static
           | Constexpr
           | Const
           | Inline
           ;

typeDecl: classDecl
        | enumDecl
        | usingDecl
        | typedefDecl
        ;

usingDecl: Using Identifier AssignEquals type Semicolon;

classDecl: template? classType classImpl? Semicolon;

//template: Template AngleString;

template: Template templateArgs?;

templateArgs: LessThan (templateIdentifier (Comma templateIdentifier)*)? GreaterThan;

templateIdentifier: (TypeName | Class | Identifier)? type;

anyIdentifier: Identifier | MacroIdentifier;

classImpl : LBrace classMembers* RBrace;

// RT Attributes

rtComment:
         ( CommentStart LBracket rtAttributes RBracket
         | RtCommentBlock rtBlockAttributes* Star* BlockCommentEnd
         );

rtBlockAttributes: Star* LBracket rtAttributes RBracket;

rtAttributes: rtAttribute (Comma rtAttribute)*;

rtAttribute: rtIdentifier (LParen arguments? RParen)?;

rtIdentifier: Identifier
            | Include
            ;

classType: (Struct | Class) interfaceType=type (Colon AccessModifier? baseType=type)?
         | DeclareRtIntf LParen interfaceType=type Comma LParen? baseType=type RParen? RParen
         ;

classMembers: preprocesor
            | macro
            | varDecl
            | typeDecl
            | methodDecl
            | constructor
            | operator
            | static_assert
            | usingDecl
            ;

methodDecl: (rtComment | docComment)*
            methodModifiers? type CallingConvention? Identifier LParen arguments? RParen Const? pureFuncDecl? Semicolon;

constructor: Identifier LParen arguments? RParen initializers? LBrace RBrace;

initializers: Colon Identifier LParen arguments RParen
             (Comma Identifier LParen arguments RParen)*;

operator: Friend? type Operator operatorType LParen arguments? RParen methodDriectives* MethodBody;

operatorType: AssignEquals AssignEquals
            | Exclamation AssignEquals
            | type
            ;

methodDriectives: Const
                | NoExcept
                ;

methodModifiers: Virtual;

pureFuncDecl: AssignEquals Int;

typedefDecl: TypeDef type type Semicolon;

enumDecl: Enum Class? Identifier enumImpl? Semicolon;

enumImpl: (Colon type)? LBrace enumMembers? Comma? RBrace;

//enumMembers: enumMember (Comma docComment? enumMember)* docComment?;

enumMembers: enumMember (Comma docComment? enumMember)* docComment?;

enumMember: Identifier (AssignEquals enumValue)?;

enumValue : cast? expr
          | MacroIdentifier
          ;

cast: LParen type RParen;

type: namespace? Identifier templateArgs? ptrOperators
    | RtInterface LParen type RParen
    ;

ptrOperators: ptrOperator*;

ptrOperator: Star | Ampersand;

namespace: (anyIdentifier DoubleColon)+;

assignment: AssignEquals expression;

expression: expression binOperator expression
          | literal
          | block
          | Quote Quote
          | namespace? Identifier
          ;

expr: LParen expression RParen
    | expression
    ;

binOperator: Ampersand
           | Pipe
           ;

literal: Int
       | HexadecimalConstant
       | BinaryConstant
       | Bool
       | String
       | NullPtr
       ;

block: LBrace expressions* RBrace;

expressions: expression (Comma expression)*;

include: Include includeName;

includeName: String | IncludePath;

macro : MacroIdentifier ((LParen BackSlash? macroArguments? BackSlash? RParen) | cast? macroArgValue )? macroEnd;

macroEnd: BackSlash macroArguments
        | Semicolon?
        ;

macroArguments: macroArg (Comma BackSlash? macroArg)*;

macroArg: namedParameter? macroArgValue;

macroArgValue: literal
             | macro
             | Const? type (Const? Identifier)?
             ;

pragma: Pragma pragmaDecl;

pragmaDecl : Identifier (LParen arguments? RParen)?;

arguments: arg (Comma arg)*;

namedParameter: Identifier Colon;

arg: namedParameter? argValue defaultArgValue?;

argValueCtor: Identifier (LParen arguments? RParen);

argValue: literal
        | type DoubleColon Identifier
        | Const? type (Const? Identifier)?
        | argValueCtor
        ;

defaultArgValue: AssignEquals literal;
