//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     ANTLR Version: 4.10.1
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

// Generated from F:/Dewesoft/C++/git/Blueberry6/shared/tools/RTGen/src\DelphiLexer.g4 by ANTLR 4.10.1

// Unreachable code detected
#pragma warning disable 0162
// The variable '...' is assigned but its value is never used
#pragma warning disable 0219
// Missing XML comment for publicly visible type or member '...'
#pragma warning disable 1591
// Ambiguous reference in cref attribute
#pragma warning disable 419

using System;
using System.IO;
using System.Text;
using Antlr4.Runtime;
using Antlr4.Runtime.Atn;
using Antlr4.Runtime.Misc;
using DFA = Antlr4.Runtime.Dfa.DFA;

[System.CodeDom.Compiler.GeneratedCode("ANTLR", "4.10.1")]
[System.CLSCompliant(false)]
public partial class DelphiLexer : Lexer {
	protected static DFA[] decisionToDFA;
	protected static PredictionContextCache sharedContextCache = new PredictionContextCache();
	public const int
		String=1, Dot=2, Unit=3, TypeBlock=4, AccessModifier=5, Interface=6, Implementation=7, 
		Uses=8, Function=9, Procedure=10, End=11, Out=12, Var=13, HashTag=14, 
		Quote=15, QuoteSingle=16, Hyphen=17, LessThan=18, GreaterThan=19, Caret=20, 
		LParen=21, RParen=22, LBrace=23, RBrace=24, LBracket=25, RBracket=26, 
		Comma=27, Slash=28, Semicolon=29, AssignEquals=30, Static=31, Const=32, 
		TypeDef=33, Struct=34, Class=35, Public=36, Private=37, Protected=38, 
		Virtual=39, Enum=40, Ampersand=41, Star=42, Colon=43, DoubleColon=44, 
		Exclamation=45, CommentStart=46, RtCommentBlock=47, BlockCommentStart=48, 
		BlockCommentEnd=49, PrimitiveValue=50, Pipe=51, CallingConvention=52, 
		VarModifiers=53, Identifier=54, HexadecimalConstant=55, Number=56, Int=57, 
		AlphaNumeric=58, Boolean=59, Whitespace=60, Comment=61, BlockComment=62, 
		Newline=63, Implementations=64, StringDouble=65;
	public const int
		WHITESPACE_CHANNEL=2, COMMENTS_CHANNEL=3;
	public static string[] channelNames = {
		"DEFAULT_TOKEN_CHANNEL", "HIDDEN", "WHITESPACE_CHANNEL", "COMMENTS_CHANNEL"
	};

	public static string[] modeNames = {
		"DEFAULT_MODE"
	};

	public static readonly string[] ruleNames = {
		"String", "Dot", "Unit", "TypeBlock", "AccessModifier", "Interface", "Implementation", 
		"Uses", "Function", "Procedure", "End", "Out", "Var", "HashTag", "Quote", 
		"QuoteSingle", "Hyphen", "LessThan", "GreaterThan", "Caret", "LParen", 
		"RParen", "LBrace", "RBrace", "LBracket", "RBracket", "Comma", "Slash", 
		"Semicolon", "AssignEquals", "Static", "Const", "TypeDef", "Struct", "Class", 
		"Public", "Private", "Protected", "Virtual", "Enum", "Ampersand", "Star", 
		"Colon", "DoubleColon", "Exclamation", "CommentStart", "RtCommentBlock", 
		"BlockCommentStart", "BlockCommentEnd", "PrimitiveValue", "Pipe", "CallingConvention", 
		"VarModifiers", "Identifier", "HexadecimalConstant", "Number", "Int", 
		"AlphaNumeric", "Boolean", "IdentifierNondigit", "Nondigit", "DigitAndCapital", 
		"Digit", "UniversalCharacterName", "HexadecimalDigitSequence", "HexadecimalDigit", 
		"HexadecimalPrefix", "HexQuad", "Letter", "Whitespace", "Comment", "BlockComment", 
		"Newline", "Implementations", "StringDouble"
	};


	public DelphiLexer(ICharStream input)
	: this(input, Console.Out, Console.Error) { }

	public DelphiLexer(ICharStream input, TextWriter output, TextWriter errorOutput)
	: base(input, output, errorOutput)
	{
		Interpreter = new LexerATNSimulator(this, _ATN, decisionToDFA, sharedContextCache);
	}

	private static readonly string[] _LiteralNames = {
		null, null, "'.'", "'unit'", "'type'", null, "'interface'", "'implementation'", 
		"'uses'", "'function'", "'procedure'", "'end'", "'out'", "'var'", "'#'", 
		"'\"'", "'''", "'-'", "'<'", "'>'", "'^'", "'('", "')'", "'{'", "'}'", 
		"'['", "']'", "','", "'/'", "';'", "'='", "'static'", "'const'", "'typedef'", 
		"'struct'", "'class'", "'public'", "'private'", "'protected'", "'virtual'", 
		"'enum'", "'&'", "'*'", "':'", "'::'", "'!'", "'//'", "'/*#'", "'/*'", 
		"'*/'", null, "'|'"
	};
	private static readonly string[] _SymbolicNames = {
		null, "String", "Dot", "Unit", "TypeBlock", "AccessModifier", "Interface", 
		"Implementation", "Uses", "Function", "Procedure", "End", "Out", "Var", 
		"HashTag", "Quote", "QuoteSingle", "Hyphen", "LessThan", "GreaterThan", 
		"Caret", "LParen", "RParen", "LBrace", "RBrace", "LBracket", "RBracket", 
		"Comma", "Slash", "Semicolon", "AssignEquals", "Static", "Const", "TypeDef", 
		"Struct", "Class", "Public", "Private", "Protected", "Virtual", "Enum", 
		"Ampersand", "Star", "Colon", "DoubleColon", "Exclamation", "CommentStart", 
		"RtCommentBlock", "BlockCommentStart", "BlockCommentEnd", "PrimitiveValue", 
		"Pipe", "CallingConvention", "VarModifiers", "Identifier", "HexadecimalConstant", 
		"Number", "Int", "AlphaNumeric", "Boolean", "Whitespace", "Comment", "BlockComment", 
		"Newline", "Implementations", "StringDouble"
	};
	public static readonly IVocabulary DefaultVocabulary = new Vocabulary(_LiteralNames, _SymbolicNames);

	[NotNull]
	public override IVocabulary Vocabulary
	{
		get
		{
			return DefaultVocabulary;
		}
	}

	public override string GrammarFileName { get { return "DelphiLexer.g4"; } }

	public override string[] RuleNames { get { return ruleNames; } }

	public override string[] ChannelNames { get { return channelNames; } }

	public override string[] ModeNames { get { return modeNames; } }

	public override int[] SerializedAtn { get { return _serializedATN; } }

	static DelphiLexer() {
		decisionToDFA = new DFA[_ATN.NumberOfDecisions];
		for (int i = 0; i < _ATN.NumberOfDecisions; i++) {
			decisionToDFA[i] = new DFA(_ATN.GetDecisionState(i), i);
		}
	}
	private static int[] _serializedATN = {
		4,0,65,576,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
		6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,
		7,14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,
		7,21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,
		7,28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,2,34,7,34,2,35,
		7,35,2,36,7,36,2,37,7,37,2,38,7,38,2,39,7,39,2,40,7,40,2,41,7,41,2,42,
		7,42,2,43,7,43,2,44,7,44,2,45,7,45,2,46,7,46,2,47,7,47,2,48,7,48,2,49,
		7,49,2,50,7,50,2,51,7,51,2,52,7,52,2,53,7,53,2,54,7,54,2,55,7,55,2,56,
		7,56,2,57,7,57,2,58,7,58,2,59,7,59,2,60,7,60,2,61,7,61,2,62,7,62,2,63,
		7,63,2,64,7,64,2,65,7,65,2,66,7,66,2,67,7,67,2,68,7,68,2,69,7,69,2,70,
		7,70,2,71,7,71,2,72,7,72,2,73,7,73,2,74,7,74,1,0,1,0,5,0,154,8,0,10,0,
		12,0,157,9,0,1,0,1,0,1,1,1,1,1,2,1,2,1,2,1,2,1,2,1,3,1,3,1,3,1,3,1,3,1,
		4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,
		1,4,1,4,1,4,1,4,3,4,195,8,4,1,5,1,5,1,5,1,5,1,5,1,5,1,5,1,5,1,5,1,5,1,
		6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,7,1,7,1,7,
		1,7,1,7,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,9,1,9,1,9,1,9,1,9,1,9,1,
		9,1,9,1,9,1,9,1,10,1,10,1,10,1,10,1,11,1,11,1,11,1,11,1,12,1,12,1,12,1,
		12,1,13,1,13,1,14,1,14,1,15,1,15,1,16,1,16,1,17,1,17,1,18,1,18,1,19,1,
		19,1,20,1,20,1,21,1,21,1,22,1,22,1,23,1,23,1,24,1,24,1,25,1,25,1,26,1,
		26,1,27,1,27,1,28,1,28,1,29,1,29,1,30,1,30,1,30,1,30,1,30,1,30,1,30,1,
		31,1,31,1,31,1,31,1,31,1,31,1,32,1,32,1,32,1,32,1,32,1,32,1,32,1,32,1,
		33,1,33,1,33,1,33,1,33,1,33,1,33,1,34,1,34,1,34,1,34,1,34,1,34,1,35,1,
		35,1,35,1,35,1,35,1,35,1,35,1,36,1,36,1,36,1,36,1,36,1,36,1,36,1,36,1,
		37,1,37,1,37,1,37,1,37,1,37,1,37,1,37,1,37,1,37,1,38,1,38,1,38,1,38,1,
		38,1,38,1,38,1,38,1,39,1,39,1,39,1,39,1,39,1,40,1,40,1,41,1,41,1,42,1,
		42,1,43,1,43,1,43,1,44,1,44,1,45,1,45,1,45,1,46,1,46,1,46,1,46,1,47,1,
		47,1,47,1,48,1,48,1,48,1,49,1,49,1,49,3,49,391,8,49,1,50,1,50,1,51,1,51,
		1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,
		1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,1,51,3,51,423,8,
		51,1,52,1,52,1,52,1,52,1,52,1,52,1,52,1,52,1,52,1,52,1,52,3,52,436,8,52,
		1,53,1,53,1,53,5,53,441,8,53,10,53,12,53,444,9,53,1,54,1,54,4,54,448,8,
		54,11,54,12,54,449,1,55,1,55,1,56,4,56,455,8,56,11,56,12,56,456,1,57,1,
		57,3,57,461,8,57,1,58,1,58,1,58,1,58,1,58,1,58,1,58,1,58,1,58,3,58,472,
		8,58,1,59,1,59,3,59,476,8,59,1,60,1,60,1,61,1,61,1,62,1,62,1,63,1,63,1,
		63,1,63,1,63,1,63,1,63,1,63,1,63,1,63,3,63,494,8,63,1,64,4,64,497,8,64,
		11,64,12,64,498,1,65,1,65,1,66,1,66,1,66,1,67,1,67,1,67,1,67,1,67,3,67,
		511,8,67,1,68,1,68,1,69,4,69,516,8,69,11,69,12,69,517,1,69,1,69,1,70,1,
		70,1,70,1,70,5,70,526,8,70,10,70,12,70,529,9,70,1,70,3,70,532,8,70,1,70,
		1,70,1,70,1,70,1,71,1,71,5,71,540,8,71,10,71,12,71,543,9,71,1,71,1,71,
		1,71,1,71,1,72,1,72,3,72,551,8,72,1,72,3,72,554,8,72,1,72,1,72,1,73,1,
		73,5,73,560,8,73,10,73,12,73,563,9,73,1,73,1,73,1,73,1,74,1,74,5,74,570,
		8,74,10,74,12,74,573,9,74,1,74,1,74,4,155,541,561,571,0,75,1,1,3,2,5,3,
		7,4,9,5,11,6,13,7,15,8,17,9,19,10,21,11,23,12,25,13,27,14,29,15,31,16,
		33,17,35,18,37,19,39,20,41,21,43,22,45,23,47,24,49,25,51,26,53,27,55,28,
		57,29,59,30,61,31,63,32,65,33,67,34,69,35,71,36,73,37,75,38,77,39,79,40,
		81,41,83,42,85,43,87,44,89,45,91,46,93,47,95,48,97,49,99,50,101,51,103,
		52,105,53,107,54,109,55,111,56,113,57,115,58,117,59,119,0,121,0,123,0,
		125,0,127,0,129,0,131,0,133,0,135,0,137,0,139,60,141,61,143,62,145,63,
		147,64,149,65,1,0,9,3,0,65,90,95,95,97,122,2,0,48,57,65,90,1,0,48,57,3,
		0,48,57,65,70,97,102,2,0,88,88,120,120,4,0,65,90,95,95,97,122,128,255,
		2,0,9,9,32,32,3,0,10,10,13,13,91,91,1,0,35,35,592,0,1,1,0,0,0,0,3,1,0,
		0,0,0,5,1,0,0,0,0,7,1,0,0,0,0,9,1,0,0,0,0,11,1,0,0,0,0,13,1,0,0,0,0,15,
		1,0,0,0,0,17,1,0,0,0,0,19,1,0,0,0,0,21,1,0,0,0,0,23,1,0,0,0,0,25,1,0,0,
		0,0,27,1,0,0,0,0,29,1,0,0,0,0,31,1,0,0,0,0,33,1,0,0,0,0,35,1,0,0,0,0,37,
		1,0,0,0,0,39,1,0,0,0,0,41,1,0,0,0,0,43,1,0,0,0,0,45,1,0,0,0,0,47,1,0,0,
		0,0,49,1,0,0,0,0,51,1,0,0,0,0,53,1,0,0,0,0,55,1,0,0,0,0,57,1,0,0,0,0,59,
		1,0,0,0,0,61,1,0,0,0,0,63,1,0,0,0,0,65,1,0,0,0,0,67,1,0,0,0,0,69,1,0,0,
		0,0,71,1,0,0,0,0,73,1,0,0,0,0,75,1,0,0,0,0,77,1,0,0,0,0,79,1,0,0,0,0,81,
		1,0,0,0,0,83,1,0,0,0,0,85,1,0,0,0,0,87,1,0,0,0,0,89,1,0,0,0,0,91,1,0,0,
		0,0,93,1,0,0,0,0,95,1,0,0,0,0,97,1,0,0,0,0,99,1,0,0,0,0,101,1,0,0,0,0,
		103,1,0,0,0,0,105,1,0,0,0,0,107,1,0,0,0,0,109,1,0,0,0,0,111,1,0,0,0,0,
		113,1,0,0,0,0,115,1,0,0,0,0,117,1,0,0,0,0,139,1,0,0,0,0,141,1,0,0,0,0,
		143,1,0,0,0,0,145,1,0,0,0,0,147,1,0,0,0,0,149,1,0,0,0,1,151,1,0,0,0,3,
		160,1,0,0,0,5,162,1,0,0,0,7,167,1,0,0,0,9,194,1,0,0,0,11,196,1,0,0,0,13,
		206,1,0,0,0,15,221,1,0,0,0,17,226,1,0,0,0,19,235,1,0,0,0,21,245,1,0,0,
		0,23,249,1,0,0,0,25,253,1,0,0,0,27,257,1,0,0,0,29,259,1,0,0,0,31,261,1,
		0,0,0,33,263,1,0,0,0,35,265,1,0,0,0,37,267,1,0,0,0,39,269,1,0,0,0,41,271,
		1,0,0,0,43,273,1,0,0,0,45,275,1,0,0,0,47,277,1,0,0,0,49,279,1,0,0,0,51,
		281,1,0,0,0,53,283,1,0,0,0,55,285,1,0,0,0,57,287,1,0,0,0,59,289,1,0,0,
		0,61,291,1,0,0,0,63,298,1,0,0,0,65,304,1,0,0,0,67,312,1,0,0,0,69,319,1,
		0,0,0,71,325,1,0,0,0,73,332,1,0,0,0,75,340,1,0,0,0,77,350,1,0,0,0,79,358,
		1,0,0,0,81,363,1,0,0,0,83,365,1,0,0,0,85,367,1,0,0,0,87,369,1,0,0,0,89,
		372,1,0,0,0,91,374,1,0,0,0,93,377,1,0,0,0,95,381,1,0,0,0,97,384,1,0,0,
		0,99,390,1,0,0,0,101,392,1,0,0,0,103,422,1,0,0,0,105,435,1,0,0,0,107,437,
		1,0,0,0,109,445,1,0,0,0,111,451,1,0,0,0,113,454,1,0,0,0,115,460,1,0,0,
		0,117,471,1,0,0,0,119,475,1,0,0,0,121,477,1,0,0,0,123,479,1,0,0,0,125,
		481,1,0,0,0,127,493,1,0,0,0,129,496,1,0,0,0,131,500,1,0,0,0,133,502,1,
		0,0,0,135,505,1,0,0,0,137,512,1,0,0,0,139,515,1,0,0,0,141,521,1,0,0,0,
		143,537,1,0,0,0,145,553,1,0,0,0,147,557,1,0,0,0,149,567,1,0,0,0,151,155,
		5,39,0,0,152,154,9,0,0,0,153,152,1,0,0,0,154,157,1,0,0,0,155,156,1,0,0,
		0,155,153,1,0,0,0,156,158,1,0,0,0,157,155,1,0,0,0,158,159,5,39,0,0,159,
		2,1,0,0,0,160,161,5,46,0,0,161,4,1,0,0,0,162,163,5,117,0,0,163,164,5,110,
		0,0,164,165,5,105,0,0,165,166,5,116,0,0,166,6,1,0,0,0,167,168,5,116,0,
		0,168,169,5,121,0,0,169,170,5,112,0,0,170,171,5,101,0,0,171,8,1,0,0,0,
		172,173,5,112,0,0,173,174,5,117,0,0,174,175,5,98,0,0,175,176,5,108,0,0,
		176,177,5,105,0,0,177,195,5,99,0,0,178,179,5,112,0,0,179,180,5,114,0,0,
		180,181,5,105,0,0,181,182,5,118,0,0,182,183,5,97,0,0,183,184,5,116,0,0,
		184,195,5,101,0,0,185,186,5,112,0,0,186,187,5,114,0,0,187,188,5,111,0,
		0,188,189,5,116,0,0,189,190,5,101,0,0,190,191,5,99,0,0,191,192,5,116,0,
		0,192,193,5,101,0,0,193,195,5,100,0,0,194,172,1,0,0,0,194,178,1,0,0,0,
		194,185,1,0,0,0,195,10,1,0,0,0,196,197,5,105,0,0,197,198,5,110,0,0,198,
		199,5,116,0,0,199,200,5,101,0,0,200,201,5,114,0,0,201,202,5,102,0,0,202,
		203,5,97,0,0,203,204,5,99,0,0,204,205,5,101,0,0,205,12,1,0,0,0,206,207,
		5,105,0,0,207,208,5,109,0,0,208,209,5,112,0,0,209,210,5,108,0,0,210,211,
		5,101,0,0,211,212,5,109,0,0,212,213,5,101,0,0,213,214,5,110,0,0,214,215,
		5,116,0,0,215,216,5,97,0,0,216,217,5,116,0,0,217,218,5,105,0,0,218,219,
		5,111,0,0,219,220,5,110,0,0,220,14,1,0,0,0,221,222,5,117,0,0,222,223,5,
		115,0,0,223,224,5,101,0,0,224,225,5,115,0,0,225,16,1,0,0,0,226,227,5,102,
		0,0,227,228,5,117,0,0,228,229,5,110,0,0,229,230,5,99,0,0,230,231,5,116,
		0,0,231,232,5,105,0,0,232,233,5,111,0,0,233,234,5,110,0,0,234,18,1,0,0,
		0,235,236,5,112,0,0,236,237,5,114,0,0,237,238,5,111,0,0,238,239,5,99,0,
		0,239,240,5,101,0,0,240,241,5,100,0,0,241,242,5,117,0,0,242,243,5,114,
		0,0,243,244,5,101,0,0,244,20,1,0,0,0,245,246,5,101,0,0,246,247,5,110,0,
		0,247,248,5,100,0,0,248,22,1,0,0,0,249,250,5,111,0,0,250,251,5,117,0,0,
		251,252,5,116,0,0,252,24,1,0,0,0,253,254,5,118,0,0,254,255,5,97,0,0,255,
		256,5,114,0,0,256,26,1,0,0,0,257,258,5,35,0,0,258,28,1,0,0,0,259,260,5,
		34,0,0,260,30,1,0,0,0,261,262,5,39,0,0,262,32,1,0,0,0,263,264,5,45,0,0,
		264,34,1,0,0,0,265,266,5,60,0,0,266,36,1,0,0,0,267,268,5,62,0,0,268,38,
		1,0,0,0,269,270,5,94,0,0,270,40,1,0,0,0,271,272,5,40,0,0,272,42,1,0,0,
		0,273,274,5,41,0,0,274,44,1,0,0,0,275,276,5,123,0,0,276,46,1,0,0,0,277,
		278,5,125,0,0,278,48,1,0,0,0,279,280,5,91,0,0,280,50,1,0,0,0,281,282,5,
		93,0,0,282,52,1,0,0,0,283,284,5,44,0,0,284,54,1,0,0,0,285,286,5,47,0,0,
		286,56,1,0,0,0,287,288,5,59,0,0,288,58,1,0,0,0,289,290,5,61,0,0,290,60,
		1,0,0,0,291,292,5,115,0,0,292,293,5,116,0,0,293,294,5,97,0,0,294,295,5,
		116,0,0,295,296,5,105,0,0,296,297,5,99,0,0,297,62,1,0,0,0,298,299,5,99,
		0,0,299,300,5,111,0,0,300,301,5,110,0,0,301,302,5,115,0,0,302,303,5,116,
		0,0,303,64,1,0,0,0,304,305,5,116,0,0,305,306,5,121,0,0,306,307,5,112,0,
		0,307,308,5,101,0,0,308,309,5,100,0,0,309,310,5,101,0,0,310,311,5,102,
		0,0,311,66,1,0,0,0,312,313,5,115,0,0,313,314,5,116,0,0,314,315,5,114,0,
		0,315,316,5,117,0,0,316,317,5,99,0,0,317,318,5,116,0,0,318,68,1,0,0,0,
		319,320,5,99,0,0,320,321,5,108,0,0,321,322,5,97,0,0,322,323,5,115,0,0,
		323,324,5,115,0,0,324,70,1,0,0,0,325,326,5,112,0,0,326,327,5,117,0,0,327,
		328,5,98,0,0,328,329,5,108,0,0,329,330,5,105,0,0,330,331,5,99,0,0,331,
		72,1,0,0,0,332,333,5,112,0,0,333,334,5,114,0,0,334,335,5,105,0,0,335,336,
		5,118,0,0,336,337,5,97,0,0,337,338,5,116,0,0,338,339,5,101,0,0,339,74,
		1,0,0,0,340,341,5,112,0,0,341,342,5,114,0,0,342,343,5,111,0,0,343,344,
		5,116,0,0,344,345,5,101,0,0,345,346,5,99,0,0,346,347,5,116,0,0,347,348,
		5,101,0,0,348,349,5,100,0,0,349,76,1,0,0,0,350,351,5,118,0,0,351,352,5,
		105,0,0,352,353,5,114,0,0,353,354,5,116,0,0,354,355,5,117,0,0,355,356,
		5,97,0,0,356,357,5,108,0,0,357,78,1,0,0,0,358,359,5,101,0,0,359,360,5,
		110,0,0,360,361,5,117,0,0,361,362,5,109,0,0,362,80,1,0,0,0,363,364,5,38,
		0,0,364,82,1,0,0,0,365,366,5,42,0,0,366,84,1,0,0,0,367,368,5,58,0,0,368,
		86,1,0,0,0,369,370,5,58,0,0,370,371,5,58,0,0,371,88,1,0,0,0,372,373,5,
		33,0,0,373,90,1,0,0,0,374,375,5,47,0,0,375,376,5,47,0,0,376,92,1,0,0,0,
		377,378,5,47,0,0,378,379,5,42,0,0,379,380,5,35,0,0,380,94,1,0,0,0,381,
		382,5,47,0,0,382,383,5,42,0,0,383,96,1,0,0,0,384,385,5,42,0,0,385,386,
		5,47,0,0,386,98,1,0,0,0,387,391,3,117,58,0,388,391,3,113,56,0,389,391,
		3,1,0,0,390,387,1,0,0,0,390,388,1,0,0,0,390,389,1,0,0,0,391,100,1,0,0,
		0,392,393,5,124,0,0,393,102,1,0,0,0,394,395,5,99,0,0,395,396,5,100,0,0,
		396,397,5,101,0,0,397,398,5,99,0,0,398,423,5,108,0,0,399,400,5,115,0,0,
		400,401,5,116,0,0,401,402,5,100,0,0,402,403,5,99,0,0,403,404,5,97,0,0,
		404,405,5,108,0,0,405,423,5,108,0,0,406,407,5,102,0,0,407,408,5,97,0,0,
		408,409,5,115,0,0,409,410,5,116,0,0,410,411,5,99,0,0,411,412,5,97,0,0,
		412,413,5,108,0,0,413,423,5,108,0,0,414,415,5,111,0,0,415,416,5,118,0,
		0,416,417,5,101,0,0,417,418,5,114,0,0,418,419,5,108,0,0,419,420,5,111,
		0,0,420,421,5,97,0,0,421,423,5,100,0,0,422,394,1,0,0,0,422,399,1,0,0,0,
		422,406,1,0,0,0,422,414,1,0,0,0,423,104,1,0,0,0,424,425,5,115,0,0,425,
		426,5,116,0,0,426,427,5,97,0,0,427,428,5,116,0,0,428,429,5,105,0,0,429,
		436,5,99,0,0,430,431,5,99,0,0,431,432,5,111,0,0,432,433,5,110,0,0,433,
		434,5,115,0,0,434,436,5,116,0,0,435,424,1,0,0,0,435,430,1,0,0,0,436,106,
		1,0,0,0,437,442,3,119,59,0,438,441,3,119,59,0,439,441,3,125,62,0,440,438,
		1,0,0,0,440,439,1,0,0,0,441,444,1,0,0,0,442,440,1,0,0,0,442,443,1,0,0,
		0,443,108,1,0,0,0,444,442,1,0,0,0,445,447,3,133,66,0,446,448,3,131,65,
		0,447,446,1,0,0,0,448,449,1,0,0,0,449,447,1,0,0,0,449,450,1,0,0,0,450,
		110,1,0,0,0,451,452,3,125,62,0,452,112,1,0,0,0,453,455,3,125,62,0,454,
		453,1,0,0,0,455,456,1,0,0,0,456,454,1,0,0,0,456,457,1,0,0,0,457,114,1,
		0,0,0,458,461,3,125,62,0,459,461,3,121,60,0,460,458,1,0,0,0,460,459,1,
		0,0,0,461,116,1,0,0,0,462,463,5,84,0,0,463,464,5,114,0,0,464,465,5,117,
		0,0,465,472,5,101,0,0,466,467,5,70,0,0,467,468,5,97,0,0,468,469,5,108,
		0,0,469,470,5,115,0,0,470,472,5,101,0,0,471,462,1,0,0,0,471,466,1,0,0,
		0,472,118,1,0,0,0,473,476,3,121,60,0,474,476,3,127,63,0,475,473,1,0,0,
		0,475,474,1,0,0,0,476,120,1,0,0,0,477,478,7,0,0,0,478,122,1,0,0,0,479,
		480,7,1,0,0,480,124,1,0,0,0,481,482,7,2,0,0,482,126,1,0,0,0,483,484,5,
		92,0,0,484,485,5,117,0,0,485,486,1,0,0,0,486,494,3,135,67,0,487,488,5,
		92,0,0,488,489,5,85,0,0,489,490,1,0,0,0,490,491,3,135,67,0,491,492,3,135,
		67,0,492,494,1,0,0,0,493,483,1,0,0,0,493,487,1,0,0,0,494,128,1,0,0,0,495,
		497,3,131,65,0,496,495,1,0,0,0,497,498,1,0,0,0,498,496,1,0,0,0,498,499,
		1,0,0,0,499,130,1,0,0,0,500,501,7,3,0,0,501,132,1,0,0,0,502,503,5,48,0,
		0,503,504,7,4,0,0,504,134,1,0,0,0,505,506,3,131,65,0,506,510,3,131,65,
		0,507,508,3,131,65,0,508,509,3,131,65,0,509,511,1,0,0,0,510,507,1,0,0,
		0,510,511,1,0,0,0,511,136,1,0,0,0,512,513,7,5,0,0,513,138,1,0,0,0,514,
		516,7,6,0,0,515,514,1,0,0,0,516,517,1,0,0,0,517,515,1,0,0,0,517,518,1,
		0,0,0,518,519,1,0,0,0,519,520,6,69,0,0,520,140,1,0,0,0,521,522,5,47,0,
		0,522,523,5,47,0,0,523,527,1,0,0,0,524,526,8,7,0,0,525,524,1,0,0,0,526,
		529,1,0,0,0,527,525,1,0,0,0,527,528,1,0,0,0,528,531,1,0,0,0,529,527,1,
		0,0,0,530,532,5,13,0,0,531,530,1,0,0,0,531,532,1,0,0,0,532,533,1,0,0,0,
		533,534,5,10,0,0,534,535,1,0,0,0,535,536,6,70,1,0,536,142,1,0,0,0,537,
		541,5,123,0,0,538,540,8,8,0,0,539,538,1,0,0,0,540,543,1,0,0,0,541,542,
		1,0,0,0,541,539,1,0,0,0,542,544,1,0,0,0,543,541,1,0,0,0,544,545,5,125,
		0,0,545,546,1,0,0,0,546,547,6,71,1,0,547,144,1,0,0,0,548,550,5,13,0,0,
		549,551,5,10,0,0,550,549,1,0,0,0,550,551,1,0,0,0,551,554,1,0,0,0,552,554,
		5,10,0,0,553,548,1,0,0,0,553,552,1,0,0,0,554,555,1,0,0,0,555,556,6,72,
		0,0,556,146,1,0,0,0,557,561,3,13,6,0,558,560,9,0,0,0,559,558,1,0,0,0,560,
		563,1,0,0,0,561,562,1,0,0,0,561,559,1,0,0,0,562,564,1,0,0,0,563,561,1,
		0,0,0,564,565,3,21,10,0,565,566,3,3,1,0,566,148,1,0,0,0,567,571,5,34,0,
		0,568,570,9,0,0,0,569,568,1,0,0,0,570,573,1,0,0,0,571,572,1,0,0,0,571,
		569,1,0,0,0,572,574,1,0,0,0,573,571,1,0,0,0,574,575,5,34,0,0,575,150,1,
		0,0,0,24,0,155,194,390,422,435,440,442,449,456,460,471,475,493,498,510,
		517,527,531,541,550,553,561,571,2,0,2,0,0,3,0
	};

	public static readonly ATN _ATN =
		new ATNDeserializer().Deserialize(_serializedATN);


}
