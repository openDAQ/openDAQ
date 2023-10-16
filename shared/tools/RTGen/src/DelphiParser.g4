parser grammar DelphiParser;

options {
    tokenVocab = DelphiLexer;
}

start :
	unit
	Interface
	usings
	typeDecls
	implementations
	;

unit: Unit namespace Semicolon;

usings: Uses namespace (Comma namespace)* Semicolon;

typeDecls: TypeBlock (typeDecl | delegateDecl | globalFunctionDecl | globalProcedureDecl | rtComment)* ;

globalFunctionDecl: functionDecl ;

globalProcedureDecl: procedureDecl ;

delegateDecl: Identifier AssignEquals functionDecl ;

typeDecl: Identifier AssignEquals (Identifier | pointerType | enumTypeDecl | interfaceDecl) Semicolon ;

pointerType: Caret Identifier;

enumTypeDecl: LParen enumTypePart (Comma enumTypePart)* RParen ;
enumTypePart: Identifier (AssignEquals PrimitiveValue)? ;

interfaceDecl: Interface (
					LParen Identifier RParen
					guidDecl?
					(functionDecl | procedureDecl | rtComment)*
					End
				)?
				;

functionDecl: Function Identifier? (LParen functionParams RParen)? Colon Identifier Semicolon? (CallingConvention Semicolon)? ;

functionParams: (functionParam (Semicolon functionParam)* )? ;

functionParam: (Out | Const | Var)? Identifier (Colon Identifier (AssignEquals PrimitiveValue)? )? ;

procedureDecl: Procedure Identifier (LParen functionParams RParen)? Semicolon CallingConvention Semicolon;

namespace: Identifier (Dot Identifier)* ;

guidDecl: LBracket String RBracket ;


implementations: Implementations;



rtComment: CommentStart LBracket rtFuncs RBracket;
rtFuncs: rtFunc (Comma rtFunc)*;
rtFunc: Identifier (LParen rtArguments? RParen)?;
rtArguments: rtArg (Comma rtArg)*;
rtArg: rtLiteral
     | Const? Identifier
     ;

rtLiteral: Int
         | HexadecimalConstant
         | StringDouble
         ;

