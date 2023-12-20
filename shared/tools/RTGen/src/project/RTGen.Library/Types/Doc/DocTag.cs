using RTGen.Interfaces.Doc;
using System;

namespace RTGen.Types.Doc
{
    /// <summary>Represents the information about a generic documentation tag.</summary>
    [Serializable]
    public class DocTag : DocElement, IDocTag
    {
        /// <summary>Initializes a new instance of the <see cref="DocTag" /> class.</summary>
        /// <param name="tagName">The name of the documentation tag.</param>
        /// <param name="type">The type of the documentation tag if known.</param>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        public DocTag(string tagName, TagType type, string rawText) : base(ElementType.Tag, rawText)
        {
            TagName = tagName;
            TagType = type;
        }

        /// <summary>The type of the documentation tag if it is a known tag.</summary>
        public TagType TagType { get; set; }

        /// <summary>The name of the documentation tag.</summary>
        /// <example>param, brief, returns</example>
        public string TagName { get; set; }
    }
}
