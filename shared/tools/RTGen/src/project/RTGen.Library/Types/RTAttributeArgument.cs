using System;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Basic implementation of IRTArgument.</summary>
    [Serializable]
    public class RTAttributeArgument : IRTAttributeArgument
    {
        /// <summary>Creates a RTAttributeArgument with specified parameter name and value.</summary>
        /// <param name="value">The value of the parameter.</param>
        /// <param name="name">The named parameter if specified otherwise <c>null</c>.</param>
        public RTAttributeArgument(string value, string name)
        {
            Name = name;
            Value = value;

            IsNamedParameter = !string.IsNullOrEmpty(Name);
        }

        /// <summary>Whether the parameter is named.</summary>
        /// <example>[property(default: 0)] => true, [property(0)] => false</example>
        public bool IsNamedParameter { get; }

        /// <summary>The named parameter name if any otherwise <c>null</c>.</summary>
        public string Name { get; set; }

        /// <summary>The parameter value.</summary>
        public string Value { get; set; }

        /// <summary>Argument info if attribute parameters are specified in the form of {type} {name}. Otherwise <c>null</c>.</summary>
        public IArgument Type { get; set; }

        public ITypeName TypeInfo { get; set; }

        /// <inheritdoc />
        public override string ToString()
        {
            return Value;
        }
    }
}
