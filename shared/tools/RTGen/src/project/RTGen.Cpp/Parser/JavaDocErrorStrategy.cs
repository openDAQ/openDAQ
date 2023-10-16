using Antlr4.Runtime;
using RTGen.Util;

namespace RTGen.Cpp.Parser
{
    class JavaDocErrorStrategy : DefaultErrorStrategy
    {
        private readonly int _fileLineOffset;

        public JavaDocErrorStrategy(int fileLineOffset)
        {
            _fileLineOffset = fileLineOffset - 1;
        }

        /// <summary>
        /// This is called by
        /// <see cref="M:Antlr4.Runtime.DefaultErrorStrategy.ReportError(Antlr4.Runtime.Parser,Antlr4.Runtime.RecognitionException)" />
        /// when the exception is an
        /// <see cref="T:Antlr4.Runtime.InputMismatchException" />
        /// .
        /// </summary>
        /// <seealso cref="M:Antlr4.Runtime.DefaultErrorStrategy.ReportError(Antlr4.Runtime.Parser,Antlr4.Runtime.RecognitionException)" />
        /// <param name="recognizer">the parser instance</param>
        /// <param name="e">the recognition exception</param>
        protected override void ReportInputMismatch(Antlr4.Runtime.Parser recognizer, InputMismatchException e) {
            string message = string.Format("mismatched input {0} expecting {1}",
                                           this.GetTokenErrorDisplay(e.OffendingToken),
                                           e.GetExpectedTokens().ToString(recognizer.Vocabulary));

            CommonToken relativeToFile = new CommonToken(e.OffendingToken);
            relativeToFile.Line += _fileLineOffset;

            recognizer.NotifyErrorListeners(relativeToFile, message, e);
        }
    }
}
