using System;
using System.Collections.Generic;
using System.Linq;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents PropertyClass info.</summary>
    [Serializable]
    public class PropertyClass : IPropertyClass
    {
        /// <summary>Initializes a new instance of the <see cref="PropertyClass" /> class.</summary>
        public PropertyClass()
        {
            AdditionalProperties = new List<IProperty>();
        }

        /// <summary>Property class name.</summary>
        public string ClassName { get; set; }

        /// <summary>Property class parent name.</summary>
        public string ParentClassName { get; set; }

        /// <summary>The name of <c>PropertyObject</c> base implementation.</summary>
        public string ImplementationBase { get; set; }

        /// <summary>Whether the generated property object implementation is templated (generic).</summary>
        public bool IsImplementationTemplated { get; set; }

        /// <summary>Property object template.</summary>
        public string ImplementationTemplate { get; set; }

        /// <summary>Base property class parameters.</summary>
        public IList<IArgument> ConstructorArguments { get; set; }

        /// <summary>Additional properties not generated through interface methods.</summary>
        public IList<IProperty> AdditionalProperties { get; }

        /// <summary>Whether the interface does not explicitly inherit from IPropertyObject or its derived types.</summary>
        public bool PrivateImplementation { get; set; }

        /// <summary>Creates a new object that is a deep copy of the current instance. </summary>
        /// <returns>A new object that is a deep copy of this instance.</returns>
        public IPropertyClass Clone()
        {
            var propClass = new PropertyClass
            {
                ClassName = this.ClassName,
                ParentClassName = this.ParentClassName,
                ImplementationBase = this.ImplementationBase,
                IsImplementationTemplated = this.IsImplementationTemplated,
                ImplementationTemplate = this.ImplementationTemplate,
                ConstructorArguments = this.ConstructorArguments.Select(t => t.Clone()).ToList()
            };

            ((List<IProperty>)propClass.AdditionalProperties).AddRange(this.AdditionalProperties.Select(p => p.Clone()));
            return propClass;
        }
    }
}
