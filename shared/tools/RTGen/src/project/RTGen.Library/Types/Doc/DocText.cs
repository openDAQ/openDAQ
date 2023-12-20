using RTGen.Interfaces.Doc;
using System;

namespace RTGen.Types.Doc
{
    /// <summary>Represents a documentation text segment.</summary>
    [Serializable]
    public class DocText : DocElement, IDocText
    {
        /// <summary>Initializes a new instance of the <see cref="DocText" /> class.</summary>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        public DocText(string rawText) : base(ElementType.Text, rawText)
        {
        }
    }
}
