namespace RTGen.Exceptions
{
    /// <summary>Base RTAttribute exception.</summary>
    public class RTAttributeException : RTGenException
    {

        /// <summary>Initializes a new instance of the <see cref="RTAttributeException" /> class with a specified error message.</summary>
        /// <param name="message">The message that describes the error. </param>
        public RTAttributeException(string message) : base(message) {
        }
    }
}
