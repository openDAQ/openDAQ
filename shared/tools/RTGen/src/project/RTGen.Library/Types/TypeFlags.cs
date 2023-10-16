using System;
using System.Runtime.Serialization;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Contains type flags and options.</summary>
    [Serializable]
    public class TypeFlags : Options<bool>, ITypeFlags
    {
        [IgnoreDataMember] private readonly TypeName _typeName;
        [IgnoreDataMember] private readonly IAttributeInfo _attributeInfo;

        /// <summary>Creates an instance of an object.</summary>
        /// <param name="typeName">The type for which the flags are used.</param>
        public TypeFlags(TypeName typeName)
        {
            _typeName = typeName;
            _attributeInfo = _typeName.AttributeInfo;
        }

        /// <summary>If the next type should have a decorator generated.</summary>
        public bool IsDecorated { get; set; }

        /// <summary>If the SmartPtr is generic/templated.</summary>
        public bool IsGeneric
        {
            get
            {
                if (_attributeInfo.PtrMappings.TryGet(_typeName.Name, out ISmartPtr ptr))
                {
                    if (ptr.IsTemplated)
                    {
                        return true;
                    }
                }
                else if (_typeName.GenericArguments != null && _typeName.GenericArguments.Count > 0)
                {
                    return true;
                }

                return false;
            }
        }

        /// <summary>If the type is a RT CoreTypes type</summary>
        public bool IsCoreType => _attributeInfo.IsCoreType(_typeName.UnmappedName);

        /// <summary>Whether the type represents a UI Control type.</summary>
        public bool IsUiControl { get; set; }

        /// <summary>Checks if the type is specialization of a generic/templated type.</summary>
        /// <returns>Returns <c>true</c> if the type is a specialization otherwise <c>false</c>.</returns>
        public bool IsSpecialization { get; set; }

        /// <summary>Checks if the type if value-type.</summary>
        /// <returns>Returns <c>true</c> if the type is value-type otherwise <c>false</c>.</returns>
        public bool IsValueType
        {
            get
            {
                if (string.IsNullOrEmpty(_typeName.Name))
                {
                    return true;
                }

                if (_typeName.UnmappedName == "void")
                {
                    return false;
                }

                if (_attributeInfo.ValueTypes.TryGet(_typeName.UnmappedName, out bool valueType))
                {
                    return valueType;
                }

                if (_typeName.Name.Length >= 2)
                {
                    //interfaces start with 2 uppercase letters (with first one being 'I'),
                    //where non-interface type names normally don't when starting with 'I' (e.g. "InfID")
                    return !((_typeName.Name[0] == 'I') && char.IsUpper(_typeName.Name[1]));
                    //ToDo: question is: is IntfID as a struct a value-type? I would says: yes
                }

                return true;
            }
        }

        /// <summary>If is an SDK internal configuration object.</summary>
        /// <example>`ISignalConfig` is a config object for `ISignal`</example>
        public bool IsCoreConfig { get; set;  }
    }
}
