using System.Collections.Generic;

namespace RTGen.Interfaces
{
    /// <summary>Represents the enumeration type</summary>
    public interface IEnumeration
    {
        /// <summary>Enumeration type name.</summary>
        string Name { get; set; }

        /// <summary>List of enumeration options and values.</summary>
        IList<IEnumOption> Options { get; }
    }
}