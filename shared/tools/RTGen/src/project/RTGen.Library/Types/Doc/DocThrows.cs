using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents the documentation attribute declaring a possible exception.</summary>
    public class DocThrows : DocAttribute, IDocThrows
    {
        /// <summary>Initializes a new instance of the <see cref="DocThrows" /> class.</summary>
        /// <param name="exceptionName">The name of the exception.</param>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        public DocThrows(string exceptionName, string rawText) : base("throws", TagType.Throws, rawText)
        {
            ExceptionName = exceptionName;
        }

        /// <summary>The name of the specified exception.</summary>
        public string ExceptionName { get; set; }
    }
}
