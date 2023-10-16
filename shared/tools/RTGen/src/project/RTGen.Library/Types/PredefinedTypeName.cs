using System;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>TypeName that can change attribute info.</summary>
    [Serializable]
    public class PredefinedTypeName : TypeName
    {
        /// <summary>Initializes a new instance of the <see cref="TypeName" /> class.</summary>
        /// <param name="info">The file's attribute info.</param>
        /// <param name="namespaceName">The type namespace.</param>
        /// <param name="name">The type name.</param>
        /// <param name="modifiers">The type modifiers.</param>
        public PredefinedTypeName(IAttributeInfo info, string namespaceName, string name, string modifiers)
            : base(info, namespaceName, name, modifiers)
        {
        }

        /// <summary>Apply the actual attribute info.</summary>
        /// <param name="info">The actual parser's attribute info.</param>
        public void SetAttributeInfo(IAttributeInfo info)
        {
            this.AttributeInfo = info;

            if (GenericArguments != null)
            {
                foreach (ITypeName genericArgument in GenericArguments)
                {
                    ((PredefinedTypeName) genericArgument).SetAttributeInfo(info);
                }
            }

            if (PropertyClass == null)
            {
                return;
            }

            if (PropertyClass.ConstructorArguments != null)
            {
                foreach (IArgument argument in PropertyClass.ConstructorArguments)
                {
                    SetArgumentAttributeInfo(argument, info);
                }
            }

            if (PropertyClass.AdditionalProperties != null)
            {
                foreach (IProperty property in PropertyClass.AdditionalProperties)
                {
                    SetPropertyAttributeInfo(property, info);
                }
            }
        }

        private static void SetPropertyAttributeInfo(IProperty property, IAttributeInfo info)
        {
            if (property.Method != null)
            {
                SetMethodAttributeInfo(property.Method, info);
            }
        }

        private static void SetMethodAttributeInfo(IMethod method, IAttributeInfo info)
        {
            if (method.Arguments != null)
            {
                foreach (IArgument argument in method.Arguments)
                {
                    SetArgumentAttributeInfo(argument, info);
                }
            }

            ((PredefinedTypeName) method.ReturnType)?.SetAttributeInfo(info);

            if (method.Overloads != null)
            {
                foreach (IOverload overload in method.Overloads)
                {
                    SetOverloadAttributeInfo(overload, info);
                }
            }
        }

        private static void SetOverloadAttributeInfo(IOverload overload, IAttributeInfo info)
        {
            if (overload.Arguments != null)
            {
                foreach (IArgument argument in overload.Arguments)
                {
                    SetArgumentAttributeInfo(argument, info);
                }
            }

            ((PredefinedTypeName) overload.ReturnType)?.SetAttributeInfo(info);
        }

        private static void SetArgumentAttributeInfo(IArgument argument, IAttributeInfo info)
        {
            ((PredefinedTypeName) argument.Type)?.SetAttributeInfo(info);
            ((PredefinedTypeName) argument.ArrayInfo?.ElementType)?.SetAttributeInfo(info);
        }
    }
}
