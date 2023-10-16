namespace RTGen.Exceptions
{
    /// <summary>Represents a base RTGen parser exception.</summary>
    public class ParserException : RTGenException
    {

        /// <summary>Initializes a new instance of the <see cref="ParserException" /> class.</summary>
        /// <param name="message">The message that describes the error. </param>
        public ParserException(string message) : base(message)
        {
        }
    }
}
