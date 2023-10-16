using System;
using System.Runtime.Serialization;

namespace RTGen.Exceptions
{
    /// <summary>Represents base RTGen exception.</summary>
    public class RTGenException : Exception
    {
        /// <summary>Initializes a new instance of the <see cref="RTGenException" /> class with a specified error message.</summary>
        /// <param name="message">The message that describes the error. </param>
        public RTGenException(string message) : base(message)
        {
        }

        /// <summary>Initializes a new instance of the <see cref="RTGenException" /> class with a specified error message.</summary>
        /// <param name="message">The error message that explains the reason for the exception. </param>
        /// <param name="innerException">The exception that is the cause of the current exception, or a null reference (<see langword="Nothing" /> in Visual Basic) if no inner exception is specified. </param>
        public RTGenException(string message, Exception innerException) : base(message, innerException)
        {
        }

        /// <summary>Initializes a new instance of the <see cref="RTGenException" /> class with serialized data.</summary>
        /// <param name="info">The <see cref="T:System.Runtime.Serialization.SerializationInfo" /> that holds the serialized object data about the exception being thrown. </param>
        /// <param name="context">The <see cref="T:System.Runtime.Serialization.StreamingContext" /> that contains contextual information about the source or destination. </param>
        /// <exception cref="T:System.ArgumentNullException">The <paramref name="info" /> parameter is <see langword="null" />. </exception>
        /// <exception cref="T:System.Runtime.Serialization.SerializationException">The class name is <see langword="null" /> or <see cref="P:System.Exception.HResult" /> is zero (0). </exception>
        protected RTGenException(SerializationInfo info, StreamingContext context) : base(info, context)
        {
        }
    }
}
