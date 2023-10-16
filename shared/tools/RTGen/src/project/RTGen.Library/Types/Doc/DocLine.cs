using System.Collections.Generic;
using RTGen.Interfaces.Doc;
using System.Linq;

namespace RTGen.Types.Doc
{
    /// <summary>Represents a documentation comment attribute description line.</summary>
    public class DocLine : IDocLine
    {
        /// <summary>Initializes a new instance of the <see cref="DocLine" /> class.</summary>
        public DocLine()
        {
            Elements = new List<IDocElement>();
        }

        /// <summary>Raw text copy of the description line in the source attribute.</summary>
        public string FullText => string.Join(" ", Elements.Select(el => el.RawText));

        /// <summary>Text and inline attributes and tags in the description line.</summary>
        public IList<IDocElement> Elements { get; set; }

        /// <summary>Returns a string that represents the current object.</summary>
        /// <returns>A string that represents the current object.</returns>
        /// <filterpriority>2</filterpriority>
        public override string ToString()
        {
            return FullText;
        }
    }
}
