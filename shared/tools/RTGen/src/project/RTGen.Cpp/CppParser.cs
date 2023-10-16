using System.Linq;
using Antlr4.Runtime;
using Antlr4.Runtime.Tree;
using RTGen.Cpp.Parser;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Util;

namespace RTGen.Cpp
{
    // ReSharper disable once ClassNeverInstantiated.Global
    public class CppParser : IParser
    {
        public IRTFile Parse(string fileName, IParserOptions options)
        {
            ICharStream stream = CharStreams.fromPath(fileName);
            RTGen3Lexer lexer = new RTGen3Lexer(stream);

            CommonTokenStream tokens = new CommonTokenStream(lexer);
            RTGen3 parser = new RTGen3(tokens) {
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

            RTGenListener listener = new RTGenListener(options);
            ParseTreeWalker.Default.Walk(listener, tree);

            ScanFileFeatures(listener.File);
            return listener.File;
        }

        public ParsedFile FileType => ParsedFile.Interface;

        private static void ScanFileFeatures(IRTFile rtFile)
        {
            FileFeatures features = FileFeatures.None;

            foreach (IRTInterface rtClass in rtFile.Classes)
            {
                if (rtClass.Events.Count > 0)
                {
                    features |= FileFeatures.Events;
                }

                if (rtClass.Type.ControlTagName != null)
                {
                    features |= FileFeatures.Controls;
                }

                if (!features.HasFlag(FileFeatures.Span))
                {
                    foreach (IMethod method in rtClass.Methods)
                    {
                        if (method.Arguments.Any(argument => argument.ArrayInfo != null))
                        {
                            features |= FileFeatures.Span;
                        }
                    }
                }
            }

            rtFile.AdditionalFeatures = features;
        }
    }
}
