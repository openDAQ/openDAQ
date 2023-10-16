using System.Collections.Generic;
using RTGen.Exceptions;

namespace RTGen.Interfaces
{
    /// <summary>Represents the information parsed from RT Comments and attributes.</summary>
    public interface IAttributeInfo : IEnumerable<KeyValuePair<string, IList<IRTAttribute>>>
    {
        /// <summary>A map of all attributes in the file by name.</summary>
        IDictionary<string, IList<IRTAttribute>> Attributes { get; }

        /// <summary>List of additional headers to add to the output file.</summary>
        IList<string> AdditionalHeaders { get; }

        /// <summary>List of additional includes/usings/uses to add to the output SmartPtr file.</summary>
        IList<ITypeName> AdditionalPtrIncludes { get; }

        /// <summary>List of additional includes/usings/uses to add to the output file.</summary>
        IList<ITypeName> AdditionalIncludes { get; }

        /// <summary>Gets the custom SmartPtr class name suffix.</summary>
        /// <returns>Returns <c>null</c> if default, otherwise the suffix string.</returns>
        /// <example><c>null</c> => Generator's decision, "Pointer" => InterfaceName<c>Pointer</c></example>
        string PtrSuffix { get; set; }

        /// <summary>Check if the type with the specified name is a value-type.</summary>
        /// <returns>Returns <c>true</c> if the specified type is a value-type otherwise <c>false</c>.</returns>
        IOptions<bool> ValueTypes { get; }

        /// <summary>Gets the custom include (file)name of the specified type.</summary>
        /// <returns>Returns the custom include (file)name of the type. If the type does not have a custom include the method throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the type does not have a custom include.</exception>
        IOptions<string> CustomIncludes { get; }

        /// <summary>Gets the custom smart-ptr name of the specified interface.</summary>
        /// <returns>Returns the interfaces custom smart-ptr name. If the interface does not have a custom smart-ptr mapping it throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the interface does not have a custom smart-ptr mapping.</exception>
        IOptions<ISmartPtr> PtrMappings { get; }

        /// <summary>Gets the namespace override of the specified type name.</summary>
        /// <returns>Returns the types namespace override info. If the type does not have a namespace override it throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the type does not have a namespace override.</exception>
        IOptions<INamespace> NamespaceOverrides { get; }

        /// <summary>Retrieves the custom XML tag name for the specified control type.</summary>
        /// <returns>Returns the name of the custom XML tag, otherwise throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the type is not a control type or it does not have a custom name defined.</exception>
        IOptions<string> CustomTagNames { get; }

        /// <summary>Retrieves the custom flag value.</summary>
        /// <returns>Returns the flag if it is available otherwise returns an exception.</returns>
        /// <exception cref="RTGenException">Throws if the flag is not available.</exception>
        IOptions<bool> CustomFlags { get; }

        /// <summary>Retrieves the custom options value.</summary>
        /// <returns>Returns the flag if it is available otherwise returns an exception.</returns>
        /// <exception cref="RTGenException">Throws if the flag is not available.</exception>
        IOptions<object> CustomOptions { get; }

        /// <summary>Gets or sets the custom type mapping for the specified RT CoreTypes type name.</summary>
        /// <returns>Returns custom type mappings, if not found throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the type does not have a custom type mapping defined.</exception>
        IOptions<string> TypeMappings { get; }

        /// <summary>Default namespace separator to use for joining namespace components.</summary>
        /// <example>[Dewesoft, RT, Core] => Dewesoft.RT.Core or Dewesoft::RT::Core.</example>
        string NamespaceSeparator { get; set; }

        /// <summary>Default parameter separator to use between parameter declarations.</summary>
        string ParameterSeparator { get; set; }

        /// <summary>The default namespace to use if not explicitly specified.</summary>
        INamespace DefaultNamespace { get; }

        /// <summary>Gets the fully-qualified name of the smart-ptr to inherit from by default.</summary>
        /// <returns>Returns the fully-qualified name of the default smart-ptr to inherit from.</returns>
        /// <example>Dewesoft::RT::Core::ObjectPtr</example>
        string GetDefaultBasePtrFull();

        /// <summary>The name of the smart-ptr to inherit from by default.</summary>
        /// <returns>Returns the name of the default smart-ptr to inherit from.</returns>
        /// <example>ObjectPtr, ObjectPtrBase</example>
        string DefaultBasePtr { get; set; }

        /// <summary>Executes the specified RT Attribute with arguments info.</summary>
        /// <param name="attribute">The RT Attribute info (name, argument values ...)</param>
        /// <returns>Returns <c>true</c> if the attribute was handled / is supported, otherwise <c>false</c>.</returns>
        bool HandleRtAttribute(IRTAttribute attribute);

        /// <summary>Checks if the specified <paramref name="typeName"/> is a RT CoreTypes type</summary>
        /// <param name="typeName">The name of the type to check.</param>
        /// <returns>Returns <c>true</c> if the type is a CoreTypes type otherwise <c>false</c>.</returns>
        bool IsCoreType(string typeName);

        /// <summary>Default library name to use if not overriden (e.g. &lt;library/header.h&gt;)</summary>
        string DefaultLibrary { get; }

        /// <summary>Library name to use for the type instead of the default one</summary>
        /// <returns>Returns custom type to library mappings, if not found throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the type does not have a custom library name defined.</exception>
        IOptions<string> LibraryOverrides { get; }

        /// <summary>The namespace to use for CoreTypes.</summary>
        INamespace CoreNamespace { get; }

        /// <summary>Gets the namespace override of the specified name.</summary>
        /// <returns>Returns the namespace override info. If the namespace does not have an override it throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the namespace does not have an override.</exception>
        IOptions<INamespace> NamespaceNameOverrides { get; }
    }
}
