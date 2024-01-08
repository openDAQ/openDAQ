using System;
using System.Collections.Generic;
using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents the information about a documentation tag with description.</summary>
    [Serializable]
    public class DocAttribute : DocTag, IDocAttribute
    {
        /// <summary>Initializes a new instance of the <see cref="DocAttribute" /> class.</summary>
        /// <param name="tagName">The name of the documentation tag.</param>
        /// <param name="type">The type of the documentation tag if known.</param>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        public DocAttribute(string tagName, TagType type, string rawText) : base(tagName, type, rawText)
        {
            Lines = new List<IDocLine>();
        }

        /// <summary>Attribute description split on new lines.</summary>
        public IList<IDocLine> Lines { get; set; }
    }
}
