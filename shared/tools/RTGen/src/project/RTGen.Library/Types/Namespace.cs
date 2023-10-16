using System;
using System.Linq;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents a C++ namespace info.</summary>
    [Serializable]
    public class Namespace : INamespace
    {
        private string _rawNamespace;

        /// <summary>Initializes a new instance of the <see cref="Namespace" /> class.</summary>
        public Namespace(string namespaceName)
        {
            Raw = namespaceName ?? "";
        }

        /// <summary>Namespace or unit as in source file.</summary>
        public string Raw
        {
            get => _rawNamespace;
            set
            {
                _rawNamespace = value;

                Components = !string.IsNullOrEmpty(_rawNamespace)
                    ? _rawNamespace.Split(new[] { "::", "." }, StringSplitOptions.RemoveEmptyEntries)
                    : new string[0];
            }
        }

        /// <summary>Namespace parts/components.</summary>
        /// <example>For Dewesoft::RT::Core => [Dewesoft, RT, Core]</example>
        public string[] Components { get; private set; }

        /// <summary>Join the namespace parts using the specified separator.</summary>
        /// <param name="separator">The separator to use.</param>
        /// <returns>The namespace components joined using the specified separator.</returns>
        /// <example><c>[Dewesoft, RT, Core]</c> and <c>'.'</c> returns <c>Dewesoft.RT.Core</c></example>
        public string ToString(string separator)
        {
            return string.Join(separator, Components);
        }

        /// <summary>Indicates whether the current object is equal to another object of the same type.</summary>
        /// <returns>true if the current object is equal to the <paramref name="other" /> parameter; otherwise, false.</returns>
        /// <param name="other">An object to compare with this object.</param>
        public bool Equals(INamespace other)
        {
            if (other == null)
            {
                return false;
            }

            if (other.Components.Length != Components.Length)
            {
                return false;
            }

            for (var i = 0; i < Components.Length; i++)
            {
                if (other.Components[i] != Components[i])
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>Returns a string that represents the current object.</summary>
        /// <returns>A string that represents the current object.</returns>
        public override string ToString()
        {
            return Raw;
        }
    }
}
