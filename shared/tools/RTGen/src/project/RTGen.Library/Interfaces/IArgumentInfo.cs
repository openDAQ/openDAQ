using System.Collections.Generic;

namespace RTGen.Interfaces
{
    /// <summary>Represents attribute info for a specific method argument.</summary>
    public interface IArgumentInfo
    {
        /// <summary>Templated argument type parameters.</summary>
        IList<ITypeName> ElementTypes { get; }

        /// <summary>Parameter info about a pointer that is passed in as an array not an output parameter.</summary>
        IArray ArrayInfo { get; set; }

        /// <summary>True if method steals the reference to the argument.</summary>
        bool StealRef { get; set; }
    }
}
