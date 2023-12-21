using RTGen.Interfaces.Doc;
using System;

namespace RTGen.Types.Doc
{
    /// <summary>Represents a short description of a documented entity.</summary>
    [Serializable]
    public class DocBrief : DocAttribute, IDocBrief
    {
        /// <summary>Initializes a new instance of the <see cref="DocBrief" /> class.</summary>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        /// <param name="index">The tags order of appearance in the documentation comment.</param>
        public DocBrief(string rawText, int index = -1) : base("brief", TagType.Brief, rawText)
        {
            TagIndex = index;
        }

        /// <summary>The tags order of appearance in the documentation comment.</summary>
        public int TagIndex { get; }
    }
}
