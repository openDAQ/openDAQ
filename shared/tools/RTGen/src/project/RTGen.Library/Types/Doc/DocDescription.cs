using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents a longer and more detailed description than <see cref="DocBrief"/>.</summary>
    public class DocDescription : DocAttribute, IDocDescription
    {
        /// <summary>Initializes a new instance of the <see cref="DocDescription" /> class.</summary>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        /// <param name="index">The tags order of appearance in the documentation comment.</param>
        public DocDescription(string rawText, int index = -1) : base("", TagType.Description, rawText)
        {
            TagIndex = index;
        }

        /// <summary>The order of appearance in the documentation comment.</summary>
        public int TagIndex { get; }
    }
}
