using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using RTGen.Attributes;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Types.Option;
using RTGen.Util;

namespace RTGen.Types
{
    /// <summary>Represents the information parsed from RT Comments and attributes.</summary>
    [Serializable]
    public class AttributeInfo : IAttributeInfo
    {
        /// <summary>Raw unchanged / overriden default legacy namespace.</summary>
        public const string DEFAULT_LEGACY_NAMESPACE = "Dewesoft.RT.Core";

        /// <summary>Raw unchanged / overriden default core namespace.</summary>
        public const string DEFAULT_CORE_NAMESPACE = "daq";

        /// <summary>The macro used to start the SDK Core namespace.</summary>
        public const string DEFAULT_CORE_NAMESPACE_MACRO = "BEGIN_NAMESPACE_OPENDAQ";

        private static readonly string[] DefaultBasePtrNamespace = { DEFAULT_CORE_NAMESPACE };

        private const string FORWARD_DECLARE_MUI_WINDOW = "ForwardDeclareMuiWindow";

        /// <summary>Turn on/of default using generation for templated SmartPointers</summary>
        public const string DEFAULT_TEMPLATE_ALIAS = "defaultAlias";

        /// <summary>Turn on/of default using generation for templated SmartPointers</summary>
        public const string DEFAULT_TEMPLATE_ALIAS_NAME = "defaultAliasName";

        /// <summary>A list of RT CoreTypes reference types.</summary>
        private static readonly HashSet<string> RTReferenceTypes = new HashSet<string>
        {
            "IBaseObject",
            "IFloat",
            "IFloatObject",
            "IInteger",
            "IIntObject",
            "IBoolObject",
            "INumber",
            "IBoolean",
            "IString",
            "IBinaryData",
            "IComparable",
            "IConvertible",
            "IDeserializer",
            "IDict",
            "IErrorInfo",
            "IFreezable",
            "IFuncObject",
            "IFunction",
            "IProcObject",
            "IProcedure",
            "IIterator",
            "IIterable",
            "IList",
            "IRatio",
            "ISerializable",
            "ISerializedList",
            "ISerializedObject",
            "ISerializer",
            "IWeakRef",
            "IUpdatable",
            "IEvent",
            "IEventArgs",
            "IEventHandler",
            "ICoreType",
            "IVersionInfo"
        };

        private readonly Dictionary<string, Action<IRTAttribute>> _rtAttributeHandlers;

        private AttributeInfo()
        {
        }

        /// <summary>Initializes a new instance of the <see cref="AttributeInfo" /> class.</summary>
        public AttributeInfo(string namespaceSeparator,
                             string parameterSeparator,
                             ILibraryInfo libraryInfo,
                             INamespace coreNamespaceOverride = null)
        {
            DefaultNamespace = libraryInfo.Namespace;
            CoreNamespace = new Namespace(DEFAULT_CORE_NAMESPACE);

            NamespaceNameOverrides = new Options<INamespace>();
            if (coreNamespaceOverride != null)
            {
                NamespaceNameOverrides.Add(DEFAULT_CORE_NAMESPACE, coreNamespaceOverride);
            }

            NamespaceSeparator = namespaceSeparator;
            ParameterSeparator = parameterSeparator;
            AdditionalHeaders = new List<string>();
            AdditionalIncludes = new List<ITypeName>();
            AdditionalPtrIncludes = new List<ITypeName>();
            Next = new Next();

            ValueTypes = new ValueType();
            CustomIncludes = new Options<string>
            (
                new Dictionary<string, string>
                {
                    { "IBaseObject", "<coretypes/objectptr.h>" }
                }
            );

            PtrMappings = new PtrMappings();
            NamespaceOverrides = new Options<INamespace>();
            CustomTagNames = new Options<string>();
            CustomFlags = new Options<bool>();

            CustomOptions = new Options<object>();
            TypeMappings = new Options<string>();

            DefaultLibrary = libraryInfo.Name;
            LibraryOverrides = new LibraryOverrides(this);

            _rtAttributeHandlers = new Dictionary<string, Action<IRTAttribute>>
            {
                {"defaultPtr", IsDefaultPtrAttribute},
                {"interfaceSmartPtr", InterfaceToSmartPtr},
                {"includeHeader", IncludeHeader},
                {"includeHeaders", IncludeHeader},
                {"interfaceNamespace", InterfaceNamespace},
                {"ignore", IgnoreMethod},
                {"include", IncludeTypeAttribute },
                {"valueType", AddValueTypeAttribute},
                {"valueTypes", AddValueTypesAttribute},
                {"templated", TemplatedType},
                {"event", RtEventAttribute},
                {"uiControl", UiControlAttribute},
                {"propertyClass", PropertyClassAttribute},
                {"elementType", ArgumentElementType},
                {"templateType", ArgumentElementType},
                {"smartPtrSuffix", PtrSuffixAttribute},
                {"property", PropertyAttribute},
                {"addProperty", AddPropertyAttribute},
                {"addEnumProperty", AddEnumPropertyAttribute},
                {"decorated", DecorateAttribute},
                {"propertyClassCtorArgs", PropClassCtorArguments},
                {"interfaceLibrary", InterfaceLibrary},
                {"libraryInterfaces", LibraryInterfaces},
                {"arrayArg", ArrayPointerArgument},
                {"factory", FactorySettings},
                {"isCoreConfig", IsCoreConfigAttribute},
                {"returnSelf", ReturnSelfAttribute}
            };

            DefaultBasePtr = "ObjectPtr";
            Attributes = new Dictionary<string, IList<IRTAttribute>>();
        }

        /// <summary>Default parameter separator to use between parameter declarations.</summary>
        public string ParameterSeparator { get; set; }

        /// <summary>Default namespace separator.</summary>
        /// <example>E.g. "::" in C++ and "." in Delphi.</example>
        public string NamespaceSeparator { get; set; }

        /// <summary>The default namespace to use if not explicitly specified.</summary>
        public INamespace DefaultNamespace { get; }

        /// <summary>A map of all attributes in the file by name.</summary>
        public IDictionary<string, IList<IRTAttribute>> Attributes { get; }

        /// <summary>List of additional headers/includes/usings to add to the output file.</summary>
        public IList<string> AdditionalHeaders { get; }

        /// <summary>List of additional includes/usings/uses to add to the output file.</summary>
        public IList<ITypeName> AdditionalIncludes { get; }

        /// <summary>List of additional includes/usings/uses to add to the output SmartPtr file.</summary>
        public IList<ITypeName> AdditionalPtrIncludes { get; }

        /// <summary>Next type or method additional info.</summary>
        public Next Next { get; }

        /// <summary>Forward declare MUI IWindow SmartPointer instead of including the header.</summary>
        public bool ForwardDeclare
        {
            get => CustomFlags.Get(FORWARD_DECLARE_MUI_WINDOW);
            set => CustomFlags.Add(FORWARD_DECLARE_MUI_WINDOW, value);
        }

        /// <summary>Gets the custom SmartPtr class name suffix.</summary>
        /// <returns>Returns <c>null</c> if default, otherwise the suffix string.</returns>
        /// <example><c>null</c> => Generator's decision, "Pointer" => InterfaceName<c>Pointer</c></example>
        public string PtrSuffix { get; set; }

        /// <summary>Check if the type with the specified name is a value-type.</summary>
        /// <returns>Returns <c>true</c> if the specified type is a value-type otherwise <c>false</c>.</returns>
        public IOptions<bool> ValueTypes { get; }

        /// <summary>Gets the custom include (file)name of the specified type.</summary>
        /// <returns>Returns the custom include (file)name of the type. If the type does not have a custom include the method throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the type does not have a custom include.</exception>
        public IOptions<string> CustomIncludes { get; }

        /// <summary>Gets the custom smart-ptr name of the specified interface.</summary>
        /// <returns>Returns the interfaces custom smart-ptr name. If the interface does not have a custom smart-ptr mapping it throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the interface does not have a custom smart-ptr mapping.</exception>
        public IOptions<ISmartPtr> PtrMappings { get; }

        /// <summary>Gets the namespace override of the specified type name.</summary>
        /// <returns>Returns the types namespace override info. If the type does not have a namespace override it throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the type does not have a namespace override.</exception>
        public IOptions<INamespace> NamespaceOverrides { get; }

        /// <summary>Gets the namespace override of the specified name.</summary>
        /// <returns>Returns the namespace override info. If the namespace does not have an override it throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the namespace does not have an override.</exception>
        public IOptions<INamespace> NamespaceNameOverrides { get; }

        /// <summary>The namespace to use for CoreTypes.</summary>
        public INamespace CoreNamespace { get; }

        /// <summary>Retrieves the custom XML tag name for the specified control type.</summary>
        /// <returns>Returns the name of the custom XML tag, otherwise throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the type is not a control type or it does not have a custom name defined.</exception>
        public IOptions<string> CustomTagNames { get; }

        /// <summary>Retrieves the custom flag value.</summary>
        /// <returns>Returns the flag if it is available otherwise returns an exception.</returns>
        /// <exception cref="RTGenException">Throws if the flag is not available.</exception>
        public IOptions<bool> CustomFlags { get; }

        /// <summary>Retrieves the custom options value.</summary>
        /// <returns>Returns the flag if it is available otherwise returns an exception.</returns>
        /// <exception cref="RTGenException">Throws if the flag is not available.</exception>
        public IOptions<object> CustomOptions { get; }

        /// <summary>Gets or sets the custom type mapping for the specified RT CoreTypes type name.</summary>
        /// <returns>Returns custom type mappings, if not found throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the type does not have a custom type mapping defined.</exception>
        public IOptions<string> TypeMappings { get; }

        #region Method argument info

        #endregion

        /// <summary>Gets the fully-qualified name of the smart-ptr to inherit from by default.</summary>
        /// <returns>Returns the fully-qualified name of the default smart-ptr to inherit from.</returns>
        /// <example>Dewesoft::RT::Core::ObjectPtr</example>
        public string GetDefaultBasePtrFull()
        {
            return string.Join(NamespaceSeparator, DefaultBasePtrNamespace) + NamespaceSeparator + DefaultBasePtr;
        }

        /// <summary>Gets the name of the smart-ptr to inherit from by default.</summary>
        /// <returns>Returns the name of the default smart-ptr to inherit from.</returns>
        /// <example>ObjectPtr</example>
        public string DefaultBasePtr { get; set; }

        /// <summary>Checks if the specified <paramref name="typeName"/> is a RT CoreTypes type</summary>
        /// <param name="typeName">The name of the type to check.</param>
        /// <returns>Returns <c>true</c> if the type is a CoreTypes type otherwise <c>false</c>.</returns>
        public bool IsCoreType(string typeName)
        {
            return ValueType.RTValueTypes.Contains(typeName) || RTReferenceTypes.Contains(typeName);
        }

        /// <summary>Default library name to use if not overriden (e.g. &lt;library/header.h&gt;)</summary>
        public string DefaultLibrary { get; }

        /// <summary>Library name to use for the type instead of the default one</summary>
        /// <returns>Returns custom type to library mappings, if not found throws an exception.</returns>
        /// <exception cref="System.Exception">Throws if the type does not have a custom library name defined.</exception>
        public IOptions<string> LibraryOverrides { get; }

        #region RtAttributes

        private void AddRtAttribute(IRTAttribute attribute, string attributeName)
        {
            if (!Attributes.ContainsKey(attributeName))
            {
                Attributes[attributeName] = new List<IRTAttribute>();
            }

            Attributes[attributeName].Add(attribute);
        }

        /// <summary>Executes the specified RT Attribute with arguments info.</summary>
        /// <param name="attribute">The RT Attribute info (name, argument values ...)</param>
        /// <returns>Returns <c>true</c> if the attribute was handled / is supported, otherwise <c>false</c>.</returns>
        public bool HandleRtAttribute(IRTAttribute attribute)
        {
            AddRtAttribute(attribute, attribute.Name);

            if (!_rtAttributeHandlers.ContainsKey(attribute.Name))
            {
                return false;
            }

            _rtAttributeHandlers[attribute.Name](attribute);

            return true;
        }

        /// <summary>Add the specified delegate as a RT Attribute handler.</summary>
        /// <param name="name">The name of the RT Attribute to handle.</param>
        /// <param name="handler">The delegate to use for attribute handling.</param>
        public void AddRtAttribute(string name, Action<IRTAttribute> handler)
        {
            _rtAttributeHandlers[name] = handler;
        }

        private void ArrayPointerArgument(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length < 2)
            {
                throw new RTAttributeException($"The attribute \"{attribute.Name}\" requires at least 1 argument name.");
            }

            string argName = attribute.Arguments[0].Value;
            string extentValue = attribute.Arguments[1].Value;

            Array array;
            if (int.TryParse(extentValue, NumberStyles.Any, CultureInfo.InvariantCulture, out int fixedExtent))
            {
                array = new Array(fixedExtent);
            }
            else if (char.IsUpper(extentValue[0]))
            {
                array = new Array(ArrayExtent.TemplateParameter, extentValue);
            }
            else
            {
                array = new Array(ArrayExtent.Parameter, extentValue);
            }

            if (!Next.Method.Arguments.Has(argName))
            {
                Next.Method.Arguments.Add(argName, new ArgumentInfo(array));
            }
            else
            {
                Next.Method.Arguments[argName].ArrayInfo = array;
            }
        }

        private void InterfaceLibrary(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length != 2)
            {
                throw new RTAttributeException($"The attribute \"{attribute.Name}\" requires 2 parameters (interface name, library name).");
            }

            LibraryOverrides.Add(attribute.Arguments[0].Value, attribute.Arguments[1].Value.Trim('\'', '"'));
        }

        private void LibraryInterfaces(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length < 2)
            {
                throw new RTAttributeException($"The attribute \"{attribute.Name}\" requires at least 2 parameters (library name and at least 1 interface).");
            }

            string libraryName = attribute.Arguments[0].Value.Trim('\'', '"');
            foreach (IRTAttributeArgument argument in attribute.Arguments.Skip(1))
            {
                LibraryOverrides.Add(argument.Value, libraryName);
            }
        }

        private void IncludeTypeAttribute(IRTAttribute attribute)
        {
            // [include(Ptr, IPropertyObjectClass)]
            if (attribute.Arguments.Length < 1)
            {
                throw new RTAttributeException($"The attribute \"{attribute.Name}\" requires at least 1 parameter (interface name).");
            }

            switch (attribute.Arguments.Length)
            {
                case 1:
                {
                    ITypeName typeName = attribute.Arguments[0].TypeInfo
                                         ?? Utility.GetTypeNameFromString(attribute.Arguments[0].Value, this);

                    AdditionalIncludes.Add(typeName);
                    break;
                }
                case 2:
                {
                    if (attribute.Arguments[0].Value == "Ptr")
                    {
                        ITypeName typeName = attribute.Arguments[1].TypeInfo
                                             ?? Utility.GetTypeNameFromString(attribute.Arguments[1].Value, this);

                        AdditionalPtrIncludes.Add(typeName);
                    }

                    break;
                }
                default:
                    throw new RTAttributeException(
                        $"The attribute \"{attribute.Name}\" requires at least 1 parameter (interface name) and include type (Ptr, Interface, ...)."
                    );
            }
        }

        private void IsDefaultPtrAttribute(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length != 1)
            {
                throw new RTAttributeException($"The attribute \"{attribute.Name}\" must have only one parameter (bool).");
            }

            if (bool.TryParse(attribute.Arguments[0].Value, out bool isDefault))
            {
                Next.Type.Set("IsDefaultPtr", isDefault);
            }
            else
            {
                throw new RTAttributeException($"The parameter to the \"{attribute.Name}\" attribute must be convertible to a boolean.");
            }
        }

        private void IsCoreConfigAttribute(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length != 1)
            {
                throw new RTAttributeException($"The attribute \"{attribute.Name}\" must have only one parameter (bool).");
            }

            if (bool.TryParse(attribute.Arguments[0].Value, out bool isConfig))
            {
                Next.Type.Set("IsCoreConfig", isConfig);
            }
            else
            {
                throw new RTAttributeException($"The parameter to the \"{attribute.Name}\" attribute must be convertible to a boolean.");
            }
        }

        private void ReturnSelfAttribute(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length != 0)
            {
                throw new RTAttributeException($"The attribute \"{attribute.Name}\" must have no parameters.");
            }

            Next.Method.ReturnSelf = true;
        }

        private void IgnoreMethod(IRTAttribute attribute)
        {
            GeneratorType ignoredFor = GeneratorType.None;

            if (attribute.Arguments.Length == 0)
            {
                foreach (GeneratorType enumValue in Enum.GetValues(typeof(GeneratorType)))
                {
                    ignoredFor |= enumValue;
                }

                ignoredFor &= ~GeneratorType.None;
            }
            else
            {
                foreach (IRTAttributeArgument argument in attribute.Arguments)
                {
                    if (Enum.TryParse(argument.Value, out GeneratorType type))
                    {
                        ignoredFor |= type;
                    }
                    else
                    {
                        throw new RTGenException($"Could not parse an enum value of GeneratorType from \"{argument.Value}\".");
                    }
                }
            }

            Next.Method.IsIgnored = ignoredFor;
        }

        private void PropClassCtorArguments(IRTAttribute attribute)
        {
            if (Next.Type.PropertyInfo == null)
            {
                Log.Warning("The \"propertyClassCtorArgs\" attribute must be defined AFTER the \"propertyClass\" attribute.");
                return;
            }

            IPropertyClass propertyClass = Next.Type.PropertyInfo;
            if (propertyClass.ConstructorArguments == null)
            {
                propertyClass.ConstructorArguments = new List<IArgument>();
            }

            foreach (IRTAttributeArgument arg in attribute.Arguments)
            {
                if (arg.Type == null)
                {
                    throw new RTAttributeException("The attribute \"propertyClassCtorArgs\" parameters must be passed in in the form {type} {name}. E.g.: \"ISomething* something\".");
                }

                propertyClass.ConstructorArguments.Add(arg.Type);
            }
        }

        private void ArgumentElementType(IRTAttribute attribute)
        {
            int argumentLength = attribute.Arguments.Length;

            if (argumentLength < 2)
            {
                throw new RTAttributeException(
                    $"RtAttribute \"{attribute.Name}\" must have at least two arguments (arg name and at least one element type).");
            }

            List<ITypeName> types = new List<ITypeName>();
            for (int i = 1; i < argumentLength; i++)
            {
                IRTAttributeArgument argument = attribute.Arguments[i];

                ITypeName attributeType = argument.Type != null
                    ? argument.Type.Type
                    : argument.TypeInfo;

                string typeName = argument.Value.Trim('"');
                types.Add(attributeType ?? GetTypeFromName(typeName));
            }

            Next.Method.Arguments.Add(attribute.Arguments[0].Value, new ArgumentInfo(types));
        }

        private void FactorySettings(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length == 0)
            {
                throw new RTAttributeException($"RtAttribute \"{attribute.Name}\" must arguments.");
            }

            foreach (IRTAttributeArgument argument in attribute.Arguments)
            {
                if (Enum.TryParse(argument.Value, out FactoryOptions option))
                {
                    Next.Factory.Options |= option;
                }
            }
        }

        private TypeName GetTypeFromName(string typeName)
        {
            return new TypeName(this, GetTypeNamespace(typeName), typeName);
        }

        private string GetTypeNamespace(string typeName)
        {
            if (string.IsNullOrEmpty(typeName))
            {
                return null;
            }

            if (NamespaceOverrides.TryGet(typeName, out INamespace namespaceOverride))
            {
                return namespaceOverride.Raw;
            }
            else
            {
                return !IsCoreType(typeName)
                           ? DefaultNamespace?.Raw
                           : DEFAULT_CORE_NAMESPACE;
            }
        }

        /// <summary>Handles the [smartPtrSuffix] attribute that defines the default SmartPtr suffix to use.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[smartPtrSuffix(defaultSuffix)]</example>
        /// <example>[smartPtrSuffix(Ptr)]</example>
        private void PtrSuffixAttribute(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length != 1)
            {
                throw new RTAttributeException("RtAttribute \"smartPtrSuffix\" must have exactly one argument.");
            }

            PtrSuffix = attribute.Arguments[0].Value.Trim('"');
        }

        private ITypeName CoreTypeName(CoreType ct)
        {
            string daqNamespace = string.Join(NamespaceSeparator, DefaultBasePtrNamespace);

            switch (ct)
            {
                case CoreType.Bool:
                    return new TypeName(this, daqNamespace, "IBoolean");
                case CoreType.Int:
                    return new TypeName(this, daqNamespace, "IInteger");
                case CoreType.Float:
                    return new TypeName(this, daqNamespace, "IFloat");
                case CoreType.String:
                    return new TypeName(this, daqNamespace, "IString");
                case CoreType.List:
                    return new TypeName(this, daqNamespace, "IList");
                case CoreType.Dictionary:
                    return new TypeName(this, daqNamespace, "IDict");
                case CoreType.Ratio:
                    return new TypeName(this, daqNamespace, "IRatio");
                case CoreType.Procedure:
                    return new TypeName(this, daqNamespace, "IProcedure");
                case CoreType.Object:
                    return new TypeName(this, daqNamespace, "IBaseObject");
                case CoreType.BinaryData:
                    return new TypeName(this, daqNamespace, "IBinaryData");
                case CoreType.Function:
                    return new TypeName(this, daqNamespace, "IFunction");
                case CoreType.Undefined:
                default:
                    throw new ArgumentOutOfRangeException(nameof(ct), ct, "Invalid base core type");
            }
        }

        /// <summary>Adds a new property to the property class.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[addProperty(IsEnabled, Bool)], [addProperty(IsEnabled, Bool, True)]</example>
        private void AddPropertyAttribute(IRTAttribute attribute)
        {
            int numArguments = attribute.Arguments.Length;

            if (numArguments < 2)
            {
                throw new RTAttributeException("RtAttribute \"addProperty\" can must have at least 2 parameters (name and type).");
            }

            if (Next.Type.PropertyInfo != null)
            {
                if (!Enum.TryParse(attribute.Arguments[1].Value, out CoreType type))
                {
                    throw new RTAttributeException("Could not parse the addProperty() type parameter as a valid CoreType. For enums use addEnumProperty().");
                }

                Property prop = new Property(attribute.Arguments[0].Value,
                                             type,
                                             CoreTypeName(type),
                                             numArguments == 3
                                                 ? attribute.Arguments[2].Value
                                                 : null
                                            );
                Next.Type.PropertyInfo.AdditionalProperties.Add(prop);
            }
        }

        /// <summary>Adds a new enum property to the property class.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[addEnumProperty(State, OperationState, 0)], [addProperty(State, OperationState, 0, List(1, 2, 3))]</example>
        private void AddEnumPropertyAttribute(IRTAttribute attribute)
        {
            int numArguments = attribute.Arguments.Length;

            if (numArguments < 3)
            {
                throw new RTAttributeException("RtAttribute \"addEnumProperty\" can must have at least 3 parameters (name, type and default value).");
            }

            if (Next.Type.PropertyInfo != null)
            {
                string defaultValue = attribute.Arguments[2].Value;
                string enumValues = numArguments >= 4 ? attribute.Arguments[3].Value : null;

                Property prop = Property.EnumProperty(
                    attribute.Arguments[0].Value,
                    defaultValue,
                    enumValues,
                    GetTypeFromName(attribute.Arguments[1].Value)
                );

                if (numArguments >= 5)
                {
                    string stringAction = attribute.Arguments[4].Value;
                    if (Enum.TryParse(stringAction, out ConfigurationAction staticConfAction))
                    {
                        prop.StaticConfigurationAction = staticConfAction;
                    }
                    else
                    {
                        throw new RTAttributeException(string.Format("Could not parse ConfigurationAction from '{1}'. Valid values are: {0}.",
                                                                     string.Join(", ", Enum.GetNames(typeof(ConfigurationAction))),
                                                                     stringAction));
                    }
                }

                Next.Type.PropertyInfo.AdditionalProperties.Add(prop);
            }
        }

        /// <summary>Changes the property defaults.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[property(Get)], [property(Set)]</example>
        private void PropertyAttribute(IRTAttribute attribute)
        {
            int numArguments = attribute.Arguments.Length;

            if (numArguments == 0)
            {
                return;
            }

            if (Next.Method.PropertyInfo == null)
            {
                Next.Method.PropertyInfo = new Property();
            }

            PropertyAttribute attributeParser = new PropertyAttribute(Next.Method.PropertyInfo);
            attributeParser.Parse(attribute.Arguments);
        }

        /// <summary>Marks the class with [propertyClass] a property class.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[propertyClass], [propertyClass(className)], [propertyClass(className, classParent)]</example>
        private void PropertyClassAttribute(IRTAttribute attribute)
        {
            const int NUM_ARGS = 5;

            int argLength = attribute.Arguments.Length;
            if (argLength > NUM_ARGS)
            {
                throw new RTAttributeException($"RtAttribute \"propertyClass\" can not have more than {NUM_ARGS} arguments.");
            }

            Next.Type.PropertyInfo = new PropertyClass();
            if (argLength == 0)
            {
                return;
            }

            PropertyClassAttribute classParser = new PropertyClassAttribute(Next.Type.PropertyInfo);
            classParser.Parse(attribute.Arguments);
        }

        /// <summary>Marks the class with [uiControl] a UI control.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[uiControl]</example>
        private void UiControlAttribute(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length > 1)
            {
                throw new RTAttributeException("RtAttribute \"uiControl\" must not have arguments.");
            }

            if (attribute.Arguments.Length == 1 && attribute.Arguments[0].Value == "ForwardDeclare")
            {
                ForwardDeclare = true;
            }

            Next.Type.IsUiControl = true;
        }

        /// <summary>Marks the method with [event] as event handler add/remove.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[event(Add, OnClick)], [event(Remove, OnClick)]</example>
        private void RtEventAttribute(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length < 2)
            {
                throw new RTAttributeException("RtAttribute \"event\" must have arguments.");
            }

            ITypeName argType = null;
            if (attribute.Arguments.Length == 3)
            {
                argType = new TypeName(this, (INamespace) null, attribute.Arguments[2].Value, null);
            }

            if (!Enum.TryParse(attribute.Arguments[0].Value, out EventMethodType addRemove))
            {
                throw new RTAttributeException(
                    $"Unknown [event] type: {attribute.Arguments[0]}. Valid options are: {Utility.EnumNames<EventMethodType>(", ")}");
            }

            Next.Method.EventInfo = new MethodEventInfo
            {
                EventName = attribute.Arguments[1].Value,
                EventArgsType = argType,
                MethodType = addRemove
            };
        }

        private void DecorateAttribute(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length == 0)
            {
                Next.Type.IsDecorated = true;
            }
            else
            {
                throw new RTAttributeException("Decorate attribute can't have parameters.");
            }
        }

        /// <summary>Handle [templated] RT Attribute. Set the interface SmartPtr as templated.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[templated] or [templated(IInterface1, IInterface2 ...)]</example>
        private void TemplatedType(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length == 0)
            {
                Next.Type.IsTemplated = true;
                return;
            }

            if (attribute.Arguments.Length <= 2 && attribute.Arguments[0].IsNamedParameter)
            {
                HandleDefaultTemplateAlias(attribute);
                return;
            }

            IRTAttributeArgument prevArgument = null;
            foreach (IRTAttributeArgument argument in attribute.Arguments)
            {
                string argumentValue = argument.Value;
                if (argument.IsNamedParameter)
                {

                    switch (argument.Name)
                    {
                        case DEFAULT_TEMPLATE_ALIAS when prevArgument != null:
                            if (bool.TryParse(argumentValue, out bool alias))
                            {
                                PtrMappings[argumentValue].HasDefaultAlias = alias;
                            }
                            else
                            {
                                throw new RTAttributeException("NoDefaultUsing parameter has to be a boolean value.");
                            }

                            break;
                        case DEFAULT_TEMPLATE_ALIAS_NAME:
                            PtrMappings.Get(argumentValue).DefaultAliasName = argumentValue;
                            break;
                        case DEFAULT_TEMPLATE_ALIAS:
                            throw new RTAttributeException("NoDefaultUsing named parameter must not be the first parameter.");
                        default:
                            throw new RTAttributeException("Templated attribute can only have \"noDefaultUsing\" named parameter.");
                    }
                }
                else
                {
                    if (PtrMappings.Has(argumentValue))
                    {
                        PtrMappings.Get(argumentValue).IsTemplated = true;
                    }
                    else
                    {
                        PtrMappings.Add(argumentValue, new SmartPtr(null, true));
                    }

                    prevArgument = argument;
                }
            }
        }

        private void HandleDefaultTemplateAlias(IRTAttribute attribute)
        {
            foreach (IRTAttributeArgument argument in attribute.Arguments)
            {
                if (!argument.IsNamedParameter)
                {
                    throw new RTAttributeException("All parameters after the first named parameter must also be named.");
                }

                switch (argument.Name)
                {
                    case DEFAULT_TEMPLATE_ALIAS:
                        Next.Type.IsTemplated = true;

                        if (bool.TryParse(argument.Value, out bool alias))
                        {
                            Next.Type.Set(DEFAULT_TEMPLATE_ALIAS, alias);
                        }
                        else
                        {
                            throw new RTAttributeException("NoDefaultUsing parameter has to be a boolean value.");
                        }

                        break;
                    case DEFAULT_TEMPLATE_ALIAS_NAME:
                        Next.Type.IsTemplated = true;
                        if (Next.Type.TryGet(DEFAULT_TEMPLATE_ALIAS, out bool hasAlias) && !hasAlias)
                        {
                            throw new RTAttributeException("Default alias name can not be changed as it will not be generated.");
                        }
                        else
                        {
                            Next.Type.Set(DEFAULT_TEMPLATE_ALIAS, true);
                        }

                        Next.Type.Set(DEFAULT_TEMPLATE_ALIAS_NAME, argument.Value);
                        break;
                }
            }
        }

        /// <summary>Handle [includeHeader] RT Attribute. Add include to the output file.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[includeHeader] or [includeHeader("some_header.h")]</example>
        private void IncludeHeader(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length == 0)
            {
                Next.AddHeader = true;
                return;
            }

            Next.AddHeader = false;

            foreach (string argument in attribute.Arguments.Select(arg => arg.Value))
            {
                AdditionalHeaders.Add(argument.StartsWith("\"<")
                    ? argument.Trim('"')
                    : argument);
            }
        }

        /// <summary>Handle [interfaceSmartPtr] RT Attribute. Manipulate interface SmartPtr options.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        private void InterfaceToSmartPtr(IRTAttribute attribute)
        {
            /*
             * Valid options:
             *
             * Set interface SmartPtr name:
             * [interfaceSmartPtr(Interface, SmartPtrName)]
             *
             * Set interface SmartPtr name and header location:
             * [interfaceSmartPtr(Interface, SmartPtrName, ptrIncludeName)]
             *
             * Set interface SmartPtr name and if it is templated or not:
             * [interfaceSmartPtr(Interface, SmartPtrName, true / false)]
             *
             * Set all options at once:
             * [interfaceSmartPtr(Interface, SmartPtrName, ptrIncludeName, true / false)]
             */

            int argLength = attribute.Arguments.Length;

            if (argLength == 0)
            {
                throw new RTAttributeException("RtAttribute \"interfaceSmartPtr\" must have arguments.");
            }

            if (argLength >= 2 && argLength <= 4)
            {
                string interfaceArg = attribute.Arguments[0].Value;
                string ptrArg = attribute.Arguments[1].Value.Trim('"');

                bool templated = false;

                if (argLength == 3 && !bool.TryParse(attribute.Arguments[2].Value, out templated))
                {
                    CustomIncludes.Add(ptrArg, attribute.Arguments[2].Value.Trim('"'));
                }

                if (argLength == 4)
                {
                    CustomIncludes.Add(ptrArg, attribute.Arguments[2].Value.Trim('"'));
                    if (!bool.TryParse(attribute.Arguments[3].Value, out templated))
                    {
                        throw new RTAttributeException("RtAttribute \"interfaceSmartPtr\" with 4 parameters the last one must be a boolean.");
                    }
                }

                if (PtrMappings.Has(interfaceArg))
                {
                    PtrMappings.Get(interfaceArg).Name = ptrArg;
                }
                else
                {
                    PtrMappings.Add(interfaceArg, new SmartPtr(ptrArg, templated));
                }
            }
            else
            {
                throw new RTAttributeException("RtAttribute \"interfaceSmartPtr\" must have at least 2 arguments.");
            }
        }

        /// <summary>Handle [interfaceNamespace] RT Attribute. Specify the interface namespace.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[interfaceNamespace(IInterface, "Some::Namespace")]</example>
        private void InterfaceNamespace(IRTAttribute attribute)
        {
            string[] arguments = attribute.Arguments.Select(arg => arg.Value).ToArray();

            if (arguments.Length != 2)
            {
                throw new RTAttributeException("RtAttribute \"interfaceNamespace\" must have at exactly 2 arguments.");
            }

            NamespaceOverrides.Add(arguments[0], new Namespace(arguments[1].Trim('"')));
        }

        /// <summary>Handle [valueType] RT Attribute. Specify a value type name.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[valueType(ValueType)]</example>
        private void AddValueTypeAttribute(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length != 1)
            {
                throw new RTAttributeException("RtAttribute \"valueType\" must have only 1 argument.");
            }

            ValueTypes.Add(attribute.Arguments[0].Value, true);
        }

        /// <summary>Handle [valueTypes] RT Attribute. Specify multiple value-types by name.</summary>
        /// <param name="attribute">RT Attribute info.</param>
        /// <example>[valueTypes(ValueType1, ValueType2 ...)]</example>
        private void AddValueTypesAttribute(IRTAttribute attribute)
        {
            if (attribute.Arguments.Length == 1)
            {
                throw new RTAttributeException("RtAttribute \"valueTypes\" must have at least 1 argument.");
            }

            foreach (string valueTypeName in attribute.Arguments.Select(arg => arg.Value))
            {
                ValueTypes.Add(valueTypeName, true);
            }
        }

        #endregion

        /// <summary>Returns an enumerator that iterates through the collection.</summary>
        /// <returns>An enumerator that can be used to iterate through the collection.</returns>
        public IEnumerator<KeyValuePair<string, IList<IRTAttribute>>> GetEnumerator()
        {
            return Attributes.GetEnumerator();
        }

        /// <summary>Returns an enumerator that iterates through a collection.</summary>
        /// <returns>An <see cref="T:System.Collections.IEnumerator" /> object that can be used to iterate through the collection.</returns>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }
    }
}
