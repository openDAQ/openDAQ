using System;
using System.Collections.Generic;
using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents the full model of a documentation comment.</summary>
    [Serializable]
    public class DocComment : IDocComment
    {
        /// <summary>Initializes a new instance of the <see cref="DocComment" /> class.</summary>
        public DocComment()
        {
            Tags = new List<IDocTag>();
        }

        /// <summary>A short description of the documented entity.</summary>
        public IDocBrief Brief { get; set; }

        /// <summary>All tags and attributes contained in the comment except <see cref="Brief"/> and <see cref="Description"/>.</summary>
        /// <remarks>Ordered by their appearance in the documentation comment.</remarks>
        public IList<IDocTag> Tags { get; set; }

        /// <summary>A longer and more detailed description than <see cref="Brief"/>.</summary>
        public IDocDescription Description { get; set; }
    }
}
