using System.Collections.Generic;

namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents the full model of a documentation comment.</summary>
    public interface IDocComment
    {
        /// <summary>A short description of the documented entity.</summary>
        IDocBrief Brief { get; set; }

        /// <summary>All tags and attributes contained in the comment except <see cref="Brief"/> and <see cref="Description"/>.</summary>
        /// <remarks>Ordered by their appearance in the documentation comment.</remarks>
        IList<IDocTag> Tags { get; }

        /// <summary>A longer and more detailed description than <see cref="Brief"/>.</summary>
        IDocDescription Description { get; set; }
    }
}
