using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents an inline documentation attribute inside a description that refers to another parameter.</summary>
    public class DocParamRef : DocTag, IDocParamRef
    {
        /// <summary>Initializes a new instance of the <see cref="DocParamRef" /> class.</summary>
        /// <param name="paramName">The name of the referenced parameter.</param>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        public DocParamRef(string paramName, string rawText) : base("p", TagType.ParamRef, rawText)
        {
            ParamName = paramName;
        }

        /// <summary>The name of the referenced parameter.</summary>
        public string ParamName { get; set; }
    }
}
