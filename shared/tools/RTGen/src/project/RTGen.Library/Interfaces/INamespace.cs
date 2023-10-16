using System;

namespace RTGen.Interfaces
{
    /// <summary>Represents a namespace info.</summary>
    public interface INamespace : IEquatable<INamespace>
    {
        /// <summary>Namespace or unit as in source file.</summary>
        string Raw { get; set; }

        /// <summary>Namespace parts/components.</summary>
        /// <example>For Dewesoft::RT::Core => [Dewesoft, RT, Core]</example>
        string[] Components { get; }

        /// <summary>Join the namespace parts using the specified separator.</summary>
        /// <param name="separator">The separator to use.</param>
        /// <returns>The namespace components joined using the specified separator.</returns>
        /// <example><c>[Dewesoft, RT, Core]</c> and <c>'.'</c> returns <c>Dewesoft.RT.Core</c></example>
        string ToString(string separator);
    }
}
