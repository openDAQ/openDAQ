using System;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Util;

namespace RTGen.Types
{
    /// <summary>Represents C++ type name info.</summary>
    [Serializable]
    public class TypeName : ITypeName
    {
        internal IAttributeInfo AttributeInfo;

        /// <summary>Initializes a new instance of the <see cref="TypeName" /> class.</summary>
        /// <param name="info">The file's attribute info.</param>
        /// <param name="namespaceName">The type namespace.</param>
        /// <param name="name">The type name.</param>
        /// <param name="modifiers">The type modifiers.</param>
        public TypeName(IAttributeInfo info, string namespaceName, string name, string modifiers = "")
            : this(info, new Namespace(namespaceName), name, modifiers)
        {
        }

        /// <summary>Initializes a new instance of the <see cref="TypeName" /> class.</summary>
        /// <param name="info">The file's attribute info.</param>
        /// <param name="typeNamespace">The type namespace.</param>
        /// <param name="name">The type name.</param>
        /// <param name="modifiers">The type modifiers.</param>
        public TypeName(IAttributeInfo info, INamespace typeNamespace, string name, string modifiers = "")
        {
            AttributeInfo = info;
            UnmappedNamespace = typeNamespace;

            Wrapper = new WrapperType(this);
            Flags = new TypeFlags(this);

            Name = name;
            Modifiers = modifiers;

            IsGenericArgument = false;
        }

        /// <summary>Initializes a new instance of the <see cref="TypeName" /> class representing a template/generic argument type.</summary>
        /// <param name="info">The file's attribute info.</param>
        /// <param name="name">The type name.</param>
        public static TypeName GenericArgument(IAttributeInfo info, string name)
        {
            return new TypeName(info, (INamespace) null, name)
            {
                IsGenericArgument = true
            };
        }

        /// <summary>Name as parsed from the file (ignores type mappings).</summary>
        public string UnmappedName { get; private set; }

        /// <summary>Type namespace.</summary>
        public INamespace Namespace
        {
            get
            {
                if (AttributeInfo.NamespaceOverrides.TryGet(Name, out INamespace namespaceOverride))
                {
                    return namespaceOverride;
                }

                return AttributeInfo.NamespaceNameOverrides.TryGet(UnmappedNamespace.ToString("."), out namespaceOverride)
                    ? namespaceOverride
                    : UnmappedNamespace;
            }
        }

        /// <summary>Type namespace as parsed from the file (ignores type mappings).</summary>
        public INamespace UnmappedNamespace { get; }

        /// <summary>Type name without namespace.</summary>
        /// <example>IButton, ActionType</example>
        [IgnoreDataMember]
        public string Name
        {
            get
            {
                if (AttributeInfo.TypeMappings.TryGet(UnmappedName, out string mappedName))
                {
                    return mappedName;
                }
                return UnmappedName;
            }
            set
            {
                UnmappedName = value;

                if (Flags.IsValueType)
                {
                    NonInterfaceName = Name;
                }
                else
                {
                    NonInterfaceName = Name.ToUpperInvariant().StartsWith("I")
                                           ? UnmappedName.Substring(1)
                                           : UnmappedName;
                }
                DefaultIncludeName = GetHeaderName(NonInterfaceName);
            }
        }

        /// <summary>Gets the header name from the type name according to convention.</summary>
        /// <param name="typeName">The type name.</param>
        /// <returns>Returns conventions based header name for the specified type.</returns>
        private string GetHeaderName(string typeName)
        {
            StringBuilder sb = new StringBuilder(typeName.Length);

            int start = 0;
            if (typeName.StartsWith("DS"))
            {
                start = 2;
                sb.Append("ds");
            }

            return sb.ToLowerSnakeCase(typeName, start)
                     .ToString();
        }

        /// <summary>Recalculates UUID v5 from the interface name and namespace.</summary>
        public void CalculateGuid()
        {
            Guid = Utility.InterfaceUuid(this);
        }

        /// <summary>Wrapper (SmartPtr) info for the current type.</summary>
        [IgnoreDataMember]
        public IWrapperType Wrapper { get; }

        /// <summary>Type flags and options.</summary>
        [IgnoreDataMember]
        public ITypeFlags Flags { get; }

        /// <summary>Type globally unique identifier otherwise <c>Guid.</c><see cref="System.Guid.Empty"/>.</summary>
        public Guid Guid { get; set; }

        /// <summary>Raw type name.</summary>
        /// <example>IString => String</example>
        /// <example>IEventArgs => EventArgs</example>
        /// <example>ActionType => ActionType</example>
        /// <example>Int => Int</example>
        public string NonInterfaceName { get; private set; }

        /// <summary>Standard/conventions-based include name.</summary>
        /// <example>ICellActionEvent => cell_action_event</example>
        public string DefaultIncludeName { get; private set; }

        /// <summary>The library or module this interface belongs to. Will be used to properly include or import the containing library.</summary>
        public string LibraryName => (AttributeInfo.LibraryOverrides.TryGet(UnmappedName, out string libraryName)
                                          ? libraryName
                                          : AttributeInfo.DefaultLibrary)?.ToLowerInvariant();

        /// <summary>The type modifiers (*, &amp;).</summary>
        public string Modifiers { get; set; }

        /// <summary>Template arguments.</summary>
        /// <example><c>IString</c> in <c>List&lt;IString&gt;</c></example>
        public IList<ITypeName> GenericArguments { get; set; }

        /// <summary>If the type is a generic type placeholder.</summary>
        /// <example><c>T, U in template&lt;T, U&gt; or IInterface&lt;T, U&gt;</c></example>
        public bool IsGenericArgument { get; set; }

        /// <summary>If the type is a template or generic that can be specialized.</summary>
        public bool HasGenericArguments => GenericArguments != null && GenericArguments.Count > 0;

        /// <summary>If the type is a PropertyClass and an instance of it is a PropertyObject.</summary>
        public IPropertyClass PropertyClass { get; set; }

        /// <summary>The XML tag name of the control. (e.g. <c>IButton</c> => <c>&lt;Button&gt;</c>)</summary>
        public string ControlTagName
        {
            get
            {
                if (!Flags.IsUiControl)
                {
                    return null;
                }

                if (AttributeInfo.CustomTagNames.TryGet(Name, out string tagName))
                {
                    return tagName;
                }
                return NonInterfaceName;
            }
        }

        /// <summary>Type name without namespace but with template parameters.</summary>
        /// <example>IAxis&lt;T, U&gt;</example>
        public string GenericName(bool withNamespace = false)
        {
            if (GenericArguments == null)
            {
                return Name;
            }

            //string genericArgs = $"<{string.Join($"{AttributeInfo.ParameterSeparator} ", GenericArguments.Select(arg => arg?.Name))}>";
            string genericArgs = $"<{string.Join($", ", GenericArguments.Select(arg => arg?.Name))}>";

            return withNamespace
                       ? NameWithNamespace(genericArgs)
                       : Name + genericArgs;
        }

        /// <summary>Type name with namespace.</summary>
        /// <param name="generic">Whether to add generic/template parameters.</param>
        /// <returns>Returns the fully-qualified type name.</returns>
        public string FullName(bool generic = true)
        {
            string templateParams = null;
            if (generic && GenericArguments != null)
            {
                //string genericArgs = string.Join($"{AttributeInfo.ParameterSeparator} ", GenericArguments.Select(arg => arg?.FullName()));
                string genericArgs = string.Join($", ", GenericArguments.Select(arg => arg?.FullName()));

                if (!string.IsNullOrEmpty(genericArgs))
                {
                    templateParams += $"<{genericArgs}>";
                }
            }
            
            return NameWithNamespace(templateParams);
        }

        private string NameWithNamespace(string templateParams)
        {
            if (Namespace.Components.Length == 0)
            {
                return Name + templateParams;
            }

            return string.Join(AttributeInfo.NamespaceSeparator, Namespace.Components) + AttributeInfo.NamespaceSeparator + Name + templateParams;
        }

        /// <summary>Gets the return type of the type depending on the type kind (value, reference).</summary>
        /// <returns>If the type is value-type returns the fully-qualified name otherwise returns the fully-qualified SmartPtr name.</returns>
        /// <example>IString => Dewesoft::RT::Core::StringPtr</example>
        /// <example>ActionType => Dewesoft::MUI::ActionType</example>
        public string ReturnType(bool fullyQualified = true)
        {
            if (Flags.IsValueType)
            {
                string fullName = fullyQualified
                                      ? FullName()
                                      : Name;

                if (Modifiers == "**")
                {
                    fullName += "*";
                }

                return fullName;
            }

            return fullyQualified
                ? Wrapper.NameFull
                : Wrapper.Name;
        }

        /// <summary>Gets the fully-qualified type name with modifiers.</summary>
        /// <returns>Returns the fully-qualified type name with modifiers.</returns>
        /// <example>Dewesoft::RT::Core::IString*</example>
        /// <example>Dewesoft::RT::Core::IString**</example>
        /// <example>Dewesoft::MUI::ActionType</example>
        /// <example>Dewesoft::MUI::ActionType*</example>
        public string WithModifiers()
        {
            return FullName() + Modifiers;
        }

        /// <summary>Checks if the custom attribute is available.</summary>
        /// <param name="property">The name of the property.</param>
        /// <returns>Returns <c>true</c> if the property is available otherwise <c>false</c>.</returns>
        public bool HasCustomProperty(string property)
        {
            return false;
        }

        /// <summary>Retrieves the custom attribute value.</summary>
        /// <param name="property">The name of the property.</param>
        /// <typeparam name="T">The type of the property value.</typeparam>
        /// <returns>Returns the property is available otherwise returns an exception.</returns>
        /// <exception cref="RTGenException">Throws when the attribute is not available.</exception>
        public T GetCustomProperty<T>(string property)
        {
            throw new RTGenException($"Unknown custom property {property}");
        }

        /// <summary>Deep clone the type info.</summary>
        /// <returns>A deep clone of the type info.</returns>
        public ITypeName Clone()
        {
            return new TypeName(this.AttributeInfo, UnmappedNamespace?.Raw, UnmappedName, Modifiers)
            {
                IsGenericArgument = this.IsGenericArgument,
                GenericArguments = this.GenericArguments?.Select(t => t.Clone()).ToList(),
                Guid = this.Guid
            };
        }

        /// <summary>Indicates whether the current object is equal to another object of the same type.</summary>
        /// <returns>true if the current object is equal to the <paramref name="other" /> parameter; otherwise, false.</returns>
        /// <param name="other">An object to compare with this object.</param>
        public bool Equals(ITypeName other)
        {
            if (other == null)
            {
                return false;
            }

            return UnmappedName == other.UnmappedName &&
                   Modifiers == other.Modifiers &&
                   (Namespace == other.Namespace || (Namespace != null && Namespace.Equals(other.Namespace)));
        }

        /// <summary>Human representation.</summary>
        public override string ToString()
        {
            return WithModifiers();
        }
    }
}
