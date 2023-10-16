using System.Collections.Generic;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents the enumeration option.</summary>
    public class EnumOption : IEnumOption
    {
        /// <summary>Creates an empty instance of <see cref="EnumOption"/>.</summary>
        public EnumOption()
        {
        }

        /// <summary>Creates an empty instance of <see cref="EnumOption"/>.</summary>
        /// <param name="name">The option name.</param>
        /// <param name="value">The option numeric value.</param>
        public EnumOption(string name, string value = null)
        {
            Name = name;
            Value = value;
        }

        /// <summary>Options name.</summary>
        /// <example>Spade, Clubs, Heart, Diamond.</example>
        public string Name { get; set; }

        /// <summary>Enumeration value.</summary>
        /// <example>1, 2, 3, 0xfff, 12345</example>
        public string Value { get; set; }

        /// <summary>If enum option has an explicit value (helper).</summary>
        public bool HasValue => !string.IsNullOrEmpty(Value);
    }

    /// <summary>Represents the enumeration type</summary>
    public class Enumeration : IEnumeration
    {
        /// <summary>Initializes a new instance of the <see cref="Enumeration" /> class.</summary>
        public Enumeration(string name) : this(name, new List<IEnumOption>())
        {
        }

        /// <summary>Initializes a new instance of the <see cref="Enumeration" /> class.</summary>
        public Enumeration(string name, IList<IEnumOption> options)
        {
            Name = name;
            Options = options;
        }

        /// <summary>Enumeration type name.</summary>
        public string Name { get; set; }

        /// <summary>List of enumeration options and values.</summary>
        public IList<IEnumOption> Options { get; }
    }
}
