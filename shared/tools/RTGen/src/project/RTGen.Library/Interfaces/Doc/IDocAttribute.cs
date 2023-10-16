using System.Collections.Generic;

namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents the information about a documentation tag with description.</summary>
    public interface IDocAttribute : IDocTag
    {
        /// <summary>Attribute description split on new lines.</summary>
        IList<IDocLine> Lines { get; set; }
    }
}
