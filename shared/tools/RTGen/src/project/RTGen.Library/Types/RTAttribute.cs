using System;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents Attribute info.</summary>
    [Serializable]
    public class RTAttribute : IRTAttribute
    {

        /// <summary>Initializes a new instance of the <see cref="RTAttribute" /> class.</summary>
        public RTAttribute(string name)
        {
            Name = name;
        }

        /// <summary>Attribute name.</summary>
        /// <example>includeHeader</example>
        public string Name { get; set; }

        /// <summary>Attribute arguments (unparsed, as read from file).</summary>
        public IRTAttributeArgument[] Arguments { get; set; }
    }
}
