namespace RTGen.Interfaces
{
    /// <summary>Array type and extent info.</summary>
    public interface IArray
    {
        /// <summary>The type of the array elements.</summary>
        ITypeName ElementType { get; }

        /// <summary>How is the size of the array expressed.</summary>
        ArrayExtent ExtentType { get; }

        /// <summary>Explicit array size if provided otherwise -1.</summary>
        int Size { get; }

        /// <summary>Parameter or template parameter reference if provided.</summary>
        string Reference { get; set; }

        /// <summary>Referenced parameter index in the original interface method otherwise -1.</summary>
        int ParameterIndex { get; set; }

        /// <summary>Check if the arrays have the same extent.</summary>
        /// <param name="array">The array to check for.</param>
        /// <returns>Returns <code>true</code> if equals otherwise <code>false</code>.</returns>
        bool ExtentEquals(IArray array);
    }
}
