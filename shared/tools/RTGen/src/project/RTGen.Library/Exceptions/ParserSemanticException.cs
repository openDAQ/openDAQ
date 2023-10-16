using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RTGen.Exceptions
{
    /// <summary>Represents a language semantic error.</summary>
    public class ParserSemanticException : ParserException
    {
        /// <summary>Creates a semantic exception with line and column info.</summary>
        /// <param name="line">The line number where the error occurred.</param>
        /// <param name="column">The column number where the error occurred.</param>
        /// <param name="message">The error message.</param>
        public ParserSemanticException(int line, int column, string message) : base($"Line {line},{column}: {message}.")
        {
        }
    }
}
