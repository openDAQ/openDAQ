using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents a documentation text segment.</summary>
    public class DocText : DocElement, IDocText
    {
        /// <summary>Initializes a new instance of the <see cref="DocText" /> class.</summary>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        public DocText(string rawText) : base(ElementType.Text, rawText)
        {
        }
    }
}
