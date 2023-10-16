using Antlr4.Runtime;

namespace RTGen.Delphi.Parser
{
    class DelphiErrorStrategy : DefaultErrorStrategy
    {
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
        protected override void ReportInputMismatch(Antlr4.Runtime.Parser recognizer, InputMismatchException e)
        {
            string message = $"Error occurred while parsing rule \"{recognizer.RuleNames[e.Context.RuleIndex]}\": mismatched input "
                             + GetTokenErrorDisplay(e.OffendingToken) + " expecting " + e.GetExpectedTokens().ToString(recognizer.Vocabulary);

            NotifyErrorListeners(recognizer, message, e);
        }
    }
}
