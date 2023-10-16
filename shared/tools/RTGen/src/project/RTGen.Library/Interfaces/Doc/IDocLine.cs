
using System.Collections.Generic;

namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents a documentation comment attribute description line.</summary>
    public interface IDocLine
    {
        /// <summary>Raw text copy of the description line in the source attribute.</summary>
        string FullText { get; }

        /// <summary>Text and inline attributes and tags in the description line.</summary>
        IList<IDocElement> Elements { get; }
    }
}
