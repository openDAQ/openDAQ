using System;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Array type and extent info.</summary>
    [Serializable]
    public class Array : IArray
    {
        /// <summary>Creates an array info with specified extent type.</summary>
        /// <param name="extent">The array extent type.</param>
        /// <param name="reference">The reference value.</param>
        public Array(ArrayExtent extent, string reference)
        {
            ExtentType = extent;
            Reference = reference;
            Size = -1;
            ParameterIndex = -1;
        }

        /// <summary>Creates an array info with specified extent type.</summary>
        public Array(int size)
        {
            ExtentType = ArrayExtent.Explicit;
            Reference = null;
            Size = size;
            ParameterIndex = -1;
        }

        /// <summary>The type of the array elements.</summary>
        public ITypeName ElementType { get; set; }

        /// <summary>How is the size of the array expressed.</summary>
        public ArrayExtent ExtentType { get; }

        /// <summary>Parameter or template parameter reference if provided.</summary>
        public string Reference { get; set; }

        /// <summary>Referenced parameter index in the original interface method otherwise -1.</summary>
        public int ParameterIndex { get; set; }

        /// <summary>Explicit array size if provided otherwise -1.</summary>
        public int Size { get; }

        /// <summary>Indicates whether the current object is equal to another object of the same type.</summary>
        /// <returns>true if the current object is equal to the <paramref name="other" /> parameter; otherwise, false.</returns>
        /// <param name="other">An object to compare with this object.</param>
        public bool ExtentEquals(IArray other)
        {
            if (other == null)
            {
                return false;
            }

            return ExtentType == other.ExtentType
                   && Reference == other.Reference
                   && Size == other.Size;
        }
    }
}
