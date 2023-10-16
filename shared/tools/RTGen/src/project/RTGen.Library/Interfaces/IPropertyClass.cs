using System;
using System.Collections.Generic;

namespace RTGen.Interfaces
{
    /// <summary>Represents PropertyClass info.</summary>
    public interface IPropertyClass : ICloneable<IPropertyClass>
    {
        /// <summary>Property class name.</summary>
        string ClassName { get; set; }

        /// <summary>Property class parent name.</summary>
        string ParentClassName { get; set; }

        /// <summary>The name of <c>PropertyObject</c> base implementation.</summary>
        string ImplementationBase { get; set; }

        /// <summary>Whether the generated property object implementation is templated (generic).</summary>
        bool IsImplementationTemplated { get; set; }

        /// <summary>Property object template.</summary>
        string ImplementationTemplate { get; set; }

        /// <summary>Base property class parameters.</summary>
        IList<IArgument> ConstructorArguments { get; set; }

        /// <summary>Additional properties not generated through interface methods.</summary>
        IList<IProperty> AdditionalProperties { get; }

        /// <summary>Whether the interface does not explicitly inherit from IPropertyObject or its derived types.</summary>
        bool PrivateImplementation { get; set; }
    }
}
