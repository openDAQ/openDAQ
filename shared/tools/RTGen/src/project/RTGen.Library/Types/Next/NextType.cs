using System;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Parsed information about the next type.</summary>
    [Serializable]
    public class NextType : PropertyHolder
    {
        /// <summary>If the next type smart-ptr should be templated.</summary>
        public bool IsTemplated { get; set; }

        /// <summary>If the next type should have a decorator generated.</summary>
        public bool IsDecorated { get; set; }

        /// <summary>If the next class is marked as a UI Control.</summary>
        public bool IsUiControl { get; set; }

        /// <summary>If not <c>null</c> next class is marked as a property class.</summary>
        public IPropertyClass PropertyInfo { get; set; }
    }
}
