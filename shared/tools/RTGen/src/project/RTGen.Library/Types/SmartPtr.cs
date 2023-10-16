using System;
using System.Text;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents SmartPtr info.</summary>
    [Serializable]
    public class SmartPtr : ISmartPtr
    {
        private bool _isDefaultPtr;

        /// <summary>Initializes a new instance of the <see cref="SmartPtr" /> class.</summary>
        public SmartPtr(string name, bool templated)
        {
            Name = name;
            IsTemplated = templated;
            HasDefaultAlias = templated;
            IsDefaultPtr = true;
        }

        /// <summary>Smart-pointer name.</summary>
        /// <example>StringPtr, ObjectPtr.</example>
        public string Name { get; set; }

        /// <summary>Whether the smart-pointer is templated (generic).</summary>
        /// <example>EventArgsPtr&lt;Interface&gt;.</example>
        public bool IsTemplated { get; set; }

        /// <summary>Whether to generated default template using/typedef.</summary>
        /// <example>using Control = ControlPtr&lt;&gt;;</example>
        public bool HasDefaultAlias { get; set; }

        /// <summary>The name of the default template using/typedef it the SmartPtr is templated.</summary>
        public string DefaultAliasName { get; set; }

        /// <summary>Whether to generate InterfaceToSmartPtr&lt;&gt; specialization.</summary>
        public bool IsDefaultPtr
        {
            get => _isDefaultPtr;
            set => _isDefaultPtr = value;
        }

        /// <summary>Returns a string that represents the current object.</summary>
        /// <returns>A string that represents the current object.</returns>
        public override string ToString()
        {
            return $"{Name}, templated: {IsTemplated}, defaultPtr: {IsDefaultPtr}";
        }
    }
}
