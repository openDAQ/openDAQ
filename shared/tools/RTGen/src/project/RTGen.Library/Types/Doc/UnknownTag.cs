using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents an uknown attribute without special handling.</summary>
    public class UnknownTag : DocAttribute, IUnknownTag
    {
        /// <summary>Initializes a new instance of the <see cref="DocTag" /> class.</summary>
        /// <param name="tagName">The name of the documentation tag.</param>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        public UnknownTag(string tagName, string rawText) : base(tagName, TagType.Unknown, rawText)
        {
        }
    }
}
