using System;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents a method argument.</summary>
    [Serializable]
    public class Argument : IArgument
    {
        /// <summary>Initializes a new instance of the <see cref="Argument" /> class.</summary>
        private Argument()
        {
        }

        /// <summary>Initializes a new instance of the <see cref="Argument" /> class.</summary>
        public Argument(ITypeName type, string name, bool isConst = false, IArray arrayInfo = null, bool isPolymorphic = false, bool isStealRef = false, bool allowNull = false, string rawBuffer = null)
        {
            Type = type;
            Name = name;
            IsConst = isConst;
            ArrayInfo = arrayInfo;
            IsPolymorphic = isPolymorphic;
            IsStealRef = isStealRef;
            AllowNull = allowNull;
            RawBuffer = rawBuffer;
        }

        /// <summary>Makes a copy of the argument with a new type.</summary>
        public Argument(ITypeName type, IArgument argument)
        {
            Type = type;
            Name = argument.Name;
            IsConst = argument.IsConst;
            ArrayInfo = argument.ArrayInfo;
            IsPolymorphic = argument.IsPolymorphic;
        }

        /// <summary>Argument type info.</summary>
        public ITypeName Type { get; set; }

        /// <summary>Argument name.</summary>
        public string Name { get; set; }

        /// <summary>Argument default value in the source language.</summary>
        public string DefaultValue { get; set; }

        /// <summary>If argument is initialized and set inside the method and returned back to the caller.</summary>
        /// <returns>Returns <c>true</c> if the argument is OUT/BY_REF otherwise <c>false</c>.</returns>
        public bool IsOutParam
        {
            get
            {
                string modifiers = Type.Modifiers;
                if (ArrayInfo != null)
                {
                    int index = modifiers.LastIndexOf('*');
                    if (index >= 0)
                    {
                        modifiers = modifiers.Remove(index, 1);
                    }
                }

                return modifiers.EndsWith(Type.Flags.IsValueType
                                              ? "*"
                                              : "**"
                                          , StringComparison.Ordinal);
            }
        }

        /// <summary>If the return type is a pointer/array.</summary>
        /// <returns>Returns <c>true</c> when the return type is a pointer/array otherwise <c>false</c>.</returns>
        public bool IsOutPointer => Type.Modifiers.EndsWith(Type.Flags.IsValueType
                                                                ? "**"
                                                                : "***",
                                                            StringComparison.Ordinal);

        /// <summary>Whether the argument is not able to be modified by the callee.</summary>
        public bool IsConst { get; }

        /// <summary>Parameter info about a pointer that is passed in as an array not an output parameter.</summary>
        public IArray ArrayInfo { get; }

        /// <summary>Argument could be represented by multiple types.</summary>
        public bool IsPolymorphic { get; set; }

        /// <summary>True if method steals the reference to the argument.</summary>
        public bool IsStealRef { get; set; }

        /// <summary>True if the argument can be null.</summary>
        public bool AllowNull { get; set; }

        /// <summary>The argument is raw buffer.</summary>
        public string RawBuffer { get; set; }

        /// <summary>Indicates whether the current object is equal to another object of the same type.</summary>
        /// <returns>true if the current object is equal to the <paramref name="other" /> parameter; otherwise, false.</returns>
        /// <param name="other">An object to compare with this object.</param>
        public bool Equals(IArgument other)
        {
            if (other == null)
            {
                return false;
            }

            return IsConst == other.IsConst &&
                   Type.Equals(other.Type);

        }

        /// <summary>Creates a new object that is a deep copy of the current instance. </summary>
        /// <returns>A new object that is a deep copy of this instance.</returns>
        public IArgument Clone()
        {
            return new Argument(Type.Clone(), this);
        }

        ///<summary>Returns a String which represents the object instance.</summary>
        public override string ToString()
        {
            return $"{(IsConst ? "const " : "")}{Type} {Name}";
        }
    }
}
