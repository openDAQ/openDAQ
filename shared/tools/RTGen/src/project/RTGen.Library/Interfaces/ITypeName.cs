using System;
using System.Collections.Generic;
using RTGen.Exceptions;

namespace RTGen.Interfaces
{
    /// <summary>Represents type name info.</summary>
    public interface ITypeName : IEquatable<ITypeName>, ICloneable<ITypeName>
    {
        /// <summary>Type namespace.</summary>
        INamespace Namespace { get; }

        /// <summary>Name as parsed from the file (ignores type mappings).</summary>
        string UnmappedName { get; }

        /// <summary>Type name without namespace.</summary>
        string Name { get; set; }

        /// <summary>Type <see cref="System.Guid"/> otherwise <c>Guid.</c><see cref="System.Guid.Empty"/>.</summary>
        Guid Guid { get; set; }

        /// <summary>Raw type name.</summary>
        /// <example>IString => String</example>
        /// <example>IEventArgs => EventArgs</example>
        /// <example>ActionType => ActionType</example>
        /// <example>Int => Int</example>
        string NonInterfaceName { get; }

        /// <summary>Standard/conventions-based include name.</summary>
        /// <example>ICellActionEvent => cell_action_event</example>
        string DefaultIncludeName { get; }

        /// <summary>Template arguments.</summary>
        /// <example><c>IString</c> in <c>List&lt;IString&gt;</c></example>
        IList<ITypeName> GenericArguments { get; set; }

        /// <summary>If the type is a generic type placeholder.</summary>
        /// <example><c>T, U in template&lt;T, U&gt; or IInterface&lt;T, U&gt;</c></example>
        bool IsGenericArgument { get; set; }

        /// <summary>Type flags and options.</summary>
        ITypeFlags Flags { get; }

        /// <summary>Wrapper (SmartPtr) info for the current type.</summary>
        IWrapperType Wrapper { get; }

        /// <summary>The type modifiers (*, &amp;).</summary>
        string Modifiers { get; set; }

        /// <summary>If the type is a PropertyClass and an instance of it is a PropertyObject.</summary>
        IPropertyClass PropertyClass { get; }

        /// <summary>The XML tag name of the control. (e.g. <c>IButton</c> => <c>&lt;Button&gt;</c>)</summary>
        string ControlTagName { get; }

        /// <summary>Type name with namespace.</summary>
        /// <param name="generic">Whether to add generic/template parameters.</param>
        /// <returns>Returns the fully-qualified type name.</returns>
        string FullName(bool generic = true);

        /// <summary>Gets the return type of the type depending on the type kind (value, reference).</summary>
        /// <returns>If the type is value-type returns the fully-qualified name otherwise returns the fully-qualified SmartPtr name.</returns>
        /// <example>IString => Dewesoft::RT::Core::StringPtr</example>
        /// <example>ActionType => Dewesoft::MUI::ActionType</example>
        string ReturnType(bool fullyQualified = true);

        /// <summary>Gets the fully-qualified type name with modifiers.</summary>
        /// <returns>Returns the fully-qualified type name with modifiers.</returns>
        /// <example>Dewesoft::RT::Core::IString*</example>
        /// <example>Dewesoft::RT::Core::IString**</example>
        /// <example>Dewesoft::MUI::ActionType</example>
        /// <example>Dewesoft::MUI::ActionType*</example>
        string WithModifiers();

        /// <summary>Checks if the custom attribute is available.</summary>
        /// <param name="property">The name of the property.</param>
        /// <returns>Returns <c>true</c> if the property is available otherwise <c>false</c>.</returns>
        bool HasCustomProperty(string property);

        /// <summary>Retrieves the custom attribute value.</summary>
        /// <param name="property">The name of the property.</param>
        /// <typeparam name="T">The type of the property value.</typeparam>
        /// <returns>Returns the property if it is available otherwise returns an exception.</returns>
        /// <exception cref="RTGenException">Throws when the attribute is not available.</exception>
        T GetCustomProperty<T>(string property);

        /// <summary>The library or module this interface belongs to. Will be used to properly include or import the containing library.</summary>
        string LibraryName { get; }

        /// <summary>Type name without namespace but with template parameters.</summary>
        /// <example>IAxis&lt;T, U&gt;</example>
        string GenericName(bool withNamespace = false);

        /// <summary>If the type is a template or generic that can be specialized.</summary>
        bool HasGenericArguments { get; }

        /// <summary>Type namespace as parsed from the file (ignores type mappings).</summary>
        INamespace UnmappedNamespace { get; }
    }
}
