using System;

namespace RTGen.Interfaces
{
    /// <summary>Represents a method argument.</summary>
    public interface IArgument : IEquatable<IArgument>, ICloneable<IArgument>
    {
        /// <summary>Argument type info.</summary>
        ITypeName Type { get; set; }

        /// <summary>Argument name.</summary>
        string Name { get; set; }

        /// <summary>If argument is initialized inside the method and returned back to the caller.</summary>
        /// <returns>Returns <c>true</c> if the argument is OUT/BY_REF otherwise <c>false</c>.</returns>
        bool IsOutParam { get; }

        /// <summary>If the return type is a pointer/array.</summary>
        /// <returns>Returns <c>true</c> when the return type is a pointer/array otherwise <c>false</c>.</returns>
        bool IsOutPointer { get; }

        /// <summary>Whether the argument is not able to be modified by the callee.</summary>
        bool IsConst { get; }

        /// <summary>Parameter info about a pointer that is passed in as an array not an output parameter.</summary>
        IArray ArrayInfo { get; }

        /// <summary>Argument default value in the source language.</summary>
        string DefaultValue { get; }
    }
}
