using Antlr4.Runtime;
using Antlr4.Runtime.Tree;
using RTGen.Delphi.Parser;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Util;

namespace RTGen.Delphi
{
    public class DelphiSourceParser : IParser
    {
        public IRTFile Parse(string fileName, IParserOptions options)
        {
            ICharStream stream = CharStreams.fromPath(fileName);
            var lexer = new DelphiLexer(stream);

            var tokens = new CommonTokenStream(lexer);
            var delphiParser = new DelphiParser(tokens)
            {
                BuildParseTree = true
            };

            if (Log.Verbose)
            {
                delphiParser.ErrorHandler = new DelphiErrorStrategy();
            }

            DelphiParser.StartContext tree = delphiParser.start();

            if (!options.ContinueOnParseErrors && delphiParser.NumberOfSyntaxErrors > 0)
            {
                throw new ParserException("Syntax errors occurred. Exiting.");
            }

            var delphiListener = new DelphiListener();

            ParseTreeWalker.Default.Walk(delphiListener, tree);
            return delphiListener.File;
        }

        public ParsedFile FileType => ParsedFile.Interface;
    }
}
