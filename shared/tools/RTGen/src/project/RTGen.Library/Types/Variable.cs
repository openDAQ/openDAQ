using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents a variable info.</summary>
    public class Variable : IVariable
    {
        /// <summary>Initializes a new instance of the <see cref="T:System.Object" /> class.</summary>
        public Variable()
        {
        }

        /// <summary>Initializes a new instance of the <see cref="T:System.Object" /> class.</summary>
        public Variable(ITypeName type, string name)
        {
            Type = type;
            Name = name;
        }

        /// <summary>Variable type info.</summary>
        public ITypeName Type { get; set; }


        /// <summary>Variable name.</summary>
        public string Name { get; set; }

        /// <summary>Unparsed value (after the assignment operator).</summary>
        public string Value { get; set; }
    }
}
