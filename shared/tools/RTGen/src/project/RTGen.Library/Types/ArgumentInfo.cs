using System.Collections.Generic;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents attribute info for a specific method argument.</summary>
    public class ArgumentInfo : IArgumentInfo
    {

        /// <summary>Creates a default empty implementation.</summary>
        public ArgumentInfo()
        {
        }

        /// <summary>Creates an argument info with the specified element types.</summary>
        /// <param name="elementTypes">The templated argument type parameters.</param>
        public ArgumentInfo(IList<ITypeName> elementTypes)
        {
            ElementTypes = elementTypes;
        }

        /// <summary>Creates an argument info with the specified element types.</summary>
        /// <param name="array">Additional info about an argument marked as an array.</param>
        public ArgumentInfo(IArray array)
        {
            ArrayInfo = array;
        }

        /// <summary>Parameter info about a pointer that is passed in as an array not an output parameter.</summary>
        public IArray ArrayInfo { get; set; }

        /// <summary>Templated argument type parameters.</summary>
        public IList<ITypeName> ElementTypes { get; set; }

        /// <summary>True if method steals the reference to the argument.</summary>
        public bool IsStealRef { get; set; }

        /// <summary>True if the argument can be null.</summary>
        public bool AllowNull { get; set; }

        /// <summary>The argument is raw buffer.</summary>
        public string RawBuffer { get; set; }
    }
}
