namespace RTGen.Exceptions
{
    /// <summary>Base IGenerator exception.</summary>
    public class GeneratorException : RTGenException
    {
        /// <summary>Initializes a new instance of the <see cref="GeneratorException" /> class.</summary>
        /// <param name="message">The message that describes the error. </param>
        public GeneratorException(string message) : base(message)
        {
        }
    }
}
