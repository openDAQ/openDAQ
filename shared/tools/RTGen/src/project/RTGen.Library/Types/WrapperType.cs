using System;
using System.Linq;
using System.Runtime.Serialization;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Wrapper (SmartPtr) info for an interface.</summary>
    [Serializable]
    public class WrapperType : IWrapperType
    {
        [IgnoreDataMember] private readonly TypeName _typeName;
        [IgnoreDataMember] private readonly IAttributeInfo _attributeInfo;

        /// <summary>Initializes the SmartPtr info.</summary>
        /// <param name="typeName">The base type info for which to get SmartPtr info.</param>
        public WrapperType(TypeName typeName)
        {
            _typeName = typeName;
            _attributeInfo = typeName.AttributeInfo;
        }

        /// <summary>Type SmartPtr standard/conventions-based include name.</summary>
        public string IncludeName
        {
            get
            {
                if (_attributeInfo.CustomIncludes.TryGet(NameOnly, out string ptrInclude))
                {
                    return ptrInclude;
                }
                return _typeName.DefaultIncludeName + "_ptr";
            }
        }

        /// <summary>Convention based SmartPtr name. (NonInterfaceName + Suffix)</summary>
        /// <example>IString => StringPtr</example>
        public string DefaultName
        {
            get
            {
                string ptrSuffix = _attributeInfo.PtrSuffix;
                if (!string.IsNullOrEmpty(_typeName.Namespace.Raw) && _typeName.Namespace.Equals(_attributeInfo.CoreNamespace))
                {
                    ptrSuffix = "Ptr";
                }

                return _typeName.NonInterfaceName + ptrSuffix;
            }
        }

        /// <summary>Actual SmartPtr name without namespace and generic parameters.</summary>
        public string NameOnly
        {
            get
            {
                if (_attributeInfo.PtrMappings.TryGet(_typeName.Name, out ISmartPtr ptr))
                {
                    string ptrName = ptr.Name;
                    if (!string.IsNullOrEmpty(ptrName))
                    {
                        return ptrName;
                    }
                }

                return DefaultName;
            }
        }

        /// <summary>Actual SmartPtr name without namespace.</summary>
        public string Name
        {
            get
            {
                string templateParams = null;
                if (_typeName.GenericArguments != null)
                {
                    string genericArgs = string.Join($"{_attributeInfo.ParameterSeparator} ", _typeName.GenericArguments.Select(arg => arg?.FullName()));
                
                    templateParams += $"<{genericArgs}>";
                }
                else if (_typeName.Flags.IsGeneric)
                {
                    if (_attributeInfo.PtrMappings.TryGet(_typeName.Name, out ISmartPtr overrides) && overrides.HasDefaultAlias)
                    {
                        return overrides.DefaultAliasName ?? $"{overrides.Name}<>";
                    }
                    else
                    {
                        templateParams = "<>";
                    }
                }

                return NameOnly + templateParams;
            }
        }

        /// <summary>Actual SmartPtr name with namespace but without generic parameters.</summary>
        public string NameQualified
        {
            get
            {
                if (string.IsNullOrEmpty(_typeName.Namespace.Raw))
                {
                    return Name;
                }

                // ReSharper disable once UseStringInterpolation
                return string.Format("{0}{1}{2}",
                    _typeName.Namespace.ToString(_attributeInfo.NamespaceSeparator),
                    _attributeInfo.NamespaceSeparator,
                    NameOnly
                );
            }
        }

        /// <summary>Actual SmartPtr with namespace.</summary>
        public string NameFull
        {
            get
            {
                if (string.IsNullOrEmpty(_typeName.Namespace.Raw))
                {
                    return Name;
                }

                // ReSharper disable once UseStringInterpolation
                return string.Format("{0}{1}{2}",
                    _typeName.Namespace.ToString(_attributeInfo.NamespaceSeparator),
                    _attributeInfo.NamespaceSeparator,
                    Name
                );
            }
        }

        /// <summary>Uses the SmartPtr name in RT Attributes otherwise falls back to default base SmartPtr.</summary>
        /// <example>If <c>[interfaceSmartPtr(IWidget, WidgetPtr)]</c> is defined => WidgetPtr, otherwise <c>ObjectPtr&lt;IWidget&gt;</c>.</example>
        public string BaseName
        {
            get
            {
                if (_attributeInfo.PtrMappings.TryGet(_typeName.Name, out ISmartPtr ptr))
                {
                    string ptrName = ptr.Name;
                    if (!string.IsNullOrEmpty(ptrName))
                    {
                        return ptrName;
                    }
                }

                return _attributeInfo.DefaultBasePtr;
            }
        }

        /// <summary>Uses the SmartPtr name with namespace in RT Attributes otherwise falls back to default base SmartPtr.</summary>
        /// <example>If <c>[interfaceSmartPtr(IWidget, WidgetPtr)]</c> is defined => Namespace::WidgetPtr, otherwise <c>Dewesoft::RT::Core::ObjectPtr&lt;IWidget&gt;</c>.</example>
        public string BaseNameFull
        {
            get
            {
                if (_attributeInfo.PtrMappings.TryGet(_typeName.Name, out ISmartPtr ptr))
                {
                    string ptrName = ptr.Name;
                    if (!string.IsNullOrEmpty(ptrName))
                    {
                        return ptrName;
                    }
                }

                return _attributeInfo.GetDefaultBasePtrFull();
            }
        }

        /// <summary>Returns a string that represents the current object.</summary>
        /// <returns>A string that represents the current object.</returns>
        public override string ToString()
        {
            return Name;
        }
    }
}
