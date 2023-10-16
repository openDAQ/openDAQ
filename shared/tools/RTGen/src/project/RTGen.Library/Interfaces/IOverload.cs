using System.Collections.Generic;
using System.Runtime.Serialization;

namespace RTGen.Interfaces
{
    /// <summary>Represents the type of the overload parsed or to generate.</summary>
    public enum OverloadType
    {
        /// <summary>The type is not known.</summary>
        Unknown,
        /// <summary>An interface method as it appears in the source.</summary>
        Interface,
        /// <summary>Method with different arguments.</summary>
        Overload,
        /// <summary>A wrapped method that has smart-ptr arguments instead of raw interfaces.</summary>
        Wrapper,
        /// <summary>A helper method that provides a more language-native arguments but may incur a performance loss.</summary>
        Helper,
        /// <summary>A constructor or a constructor overload.</summary>
        Constructor
    }

    /// <summary>Represents the overload to a method.</summary>
    public interface IOverload
    {
        /// <summary>List of method arguments (type, name, modifiers etc.).</summary>
        IList<IArgument> Arguments { get; set; }

        /// <summary>Type the method returns if its not void (procedure).</summary>
        ITypeName ReturnType { get; set; }

        /// <summary>The type of the overload.</summary>
        OverloadType Type { get; set; }

        /// <summary>Holds generator custom info (implementation defined).</summary>
        object Tag { get; set; }

        /// <summary>The last OUT, BY REFERENCE or return by pointer argument.</summary>
        /// <returns>Returns the last by ref argument otherwise <c>null</c>.</returns>
        IArgument GetLastByRefArgument();

        /// <summary>Checks if the method returns by reference (has an out parameter).</summary>
        /// <returns>Returns <c>true</c> if method returns by reference otherwise <c>false</c>.</returns>
        bool ReturnsByRef();

        /// <summary>The method to which this overload belongs to.</summary>
        [IgnoreDataMember]
        IMethod Method { get; set; }
    }
}
