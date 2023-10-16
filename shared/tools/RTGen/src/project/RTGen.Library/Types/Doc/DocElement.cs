using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents an element in the documentation comments.</summary>
    /// <remarks>Usually a tag or text.</remarks>
    public class DocElement : IDocElement
    {
        /// <summary>Initializes a new instance of the <see cref="DocComment" /> class.</summary>
        /// <param name="type">The type of the documentation element.</param>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        public DocElement(ElementType type, string rawText)
        {
            ElementType = type;
            RawText = rawText;
        }

        /// <summary>Raw text copy of the element in the source comment.</summary>
        public string RawText { get; set; }

        /// <summary>The type of the documentation comment element.</summary>
        public ElementType ElementType { get; set; }

        /// <summary>Returns a string that represents the current object.</summary>
        /// <returns>A string that represents the current object.</returns>
        /// <filterpriority>2</filterpriority>
        public override string ToString()
        {
            return RawText;
        }
    }
}
