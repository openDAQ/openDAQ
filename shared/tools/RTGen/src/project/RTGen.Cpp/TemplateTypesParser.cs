using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Antlr4.Runtime;
using Antlr4.Runtime.Tree;
using RTGen.Cpp.Parser;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Util;

namespace RTGen.Cpp
{
    // ReSharper disable once ClassNeverInstantiated.Global
    public class TemplateTypesParser : IParser
    {
        public IRTFile Parse(string fileName, IParserOptions options)
        {
            ICharStream stream = CharStreams.fromPath(fileName);
            RTGen3Lexer lexer = new RTGen3Lexer(stream);

            CommonTokenStream tokens = new CommonTokenStream(lexer);
            RTGen3 parser = new RTGen3(tokens)
            {
                BuildParseTree = true,
            };

            if (Log.Verbose)
            {
                parser.ErrorHandler = new RtGenErrorStrategy();
            }

            RTGen3.StartContext tree = parser.start();

            if (!options.ContinueOnParseErrors && parser.NumberOfSyntaxErrors > 0)
            {
                throw new ParserException("Syntax errors occurred. Exiting.");
            }

            RTGenTemplateTypesListener listener = new RTGenTemplateTypesListener(options);
            ParseTreeWalker.Default.Walk(listener, tree);

            return listener.File;
        }

        public ParsedFile FileType => ParsedFile.Custom;
    }
}
