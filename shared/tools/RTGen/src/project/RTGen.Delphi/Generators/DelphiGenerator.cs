using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using RTGen.Delphi.Types;
using RTGen.Generation;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;
using RTGen.Types;
using RTGen.Util;
using Argument = RTGen.Types.Argument;

namespace RTGen.Delphi.Generators
{
    enum GenerationType
    {
        Normal,
        Factory,
        FactoryWrapper,
        FactoryConstructor,
        FactoryConstructorTemplate,
        Interface,
        InterfacePtr,
        InterfaceImpl
    }

    // ReSharper disable once ClassNeverInstantiated.Global
    public class DelphiGenerator : TemplateGenerator
    {
        private const string IDENT = "    ";
        private const string ERROR_CODE_PREFIX = "OPENDAQ_ERR_";

        private IGeneratorOptions _options;
        private StringBuilder _wrappedMethods;
        private StringBuilder _wrappedInterfaceMethods;
        private StringBuilder _interfaceMethods;
        private StringBuilder _interfaceMappings;
        private StringBuilder _methodImpl;
        private StringBuilder _interfaceMethodImpl;
        private bool _isGeneratingInterfaceMethods;
        private GenerationType _generationType;
        private readonly HashSet<string> _pointerArrays;
        private readonly HashSet<string> _boxedOrdinalTypes;
        private readonly Dictionary<string, string> _interfaceToValueType;
        private readonly Dictionary<string, string> _reservedKeywords;
        private readonly Dictionary<string, string> _knownExceptions;

        public override IGeneratorOptions Options
        {
            get => _options;
            set
            {
                _options = value;
                if (string.IsNullOrEmpty(_options.GeneratedExtension))
                {
                    _options.GeneratedExtension = ".pas";
                }
            }
        }

        public override IRTFile RtFile
        {
            get => base.RtFile;
            set
            {
                base.RtFile = value;

                IRTInterface currentClass = value.CurrentClass;
                string interfaceNamespace = currentClass.Type.Namespace?.ToString(".");

                string typeName = GetUnreservedUnitName(currentClass.Type);

                string fileName = !string.IsNullOrEmpty(interfaceNamespace)
                                      ? interfaceNamespace + "." + typeName
                                      : typeName;

                Options.Filename = fileName;
            }
        }

        public override IVersionInfo Version => new VersionInfo
        {
            Major = 4,
            Minor = 0,
            Patch = 1
        };

        public DelphiGenerator()
        {
            _reservedKeywords = new Dictionary<string, string>(StringComparer.CurrentCultureIgnoreCase)
            {
                { "object", "Obj" },
                { "string", "Str" },
                { "property", "AProperty" },
                { "function", "func" },
                { "procedure", "proc" },
                { "result", "AResult" },
                { "type", "AType" },
                { "end", "AEnd" },
                { "begin", "ABegin" },
                { "record", "ARecord" },
                { "Unit", "AUnit" },
                { "then", "thenA" },
            };

            _knownExceptions = new Dictionary<string, string>
            {
                { "OPENDAQ_ERR_NOMEMORY", "NoMemory" },
                { "OPENDAQ_ERR_INVALIDPARAMETER", "InvalidParameter" },
                { "OPENDAQ_ERR_ARGUMENT_NULL", "ArgumentNull" },
                { "OPENDAQ_ERR_NOINTERFACE", "NoInterface" },
                { "OPENDAQ_ERR_NOTASSIGNED", "NotAssigned" },
                { "OPENDAQ_ERR_NOTENABLED", "NotEnabled" },
                { "OPENDAQ_ERR_OUTOFRANGE", "OutOfRange" },
                { "OPENDAQ_ERR_NOTFOUND", "NotFound" },
                { "OPENDAQ_ERR_GENERALERROR", "GeneralError" },
                { "OPENDAQ_ERR_INVALIDTYPE", "InvalidType" },
                { "OPENDAQ_ERR_CALCFAILED", "CalcFailed" },
                { "OPENDAQ_ERR_ALREADYEXISTS", "AlreadyExists" },
                { "OPENDAQ_ERR_FROZEN", "Frozen" },
                { "OPENDAQ_ERR_PARSEFAILED", "ParseFailed" },
                { "OPENDAQ_ERR_NOTIMPLEMENTED", "NotImplemented" },
                { "OPENDAQ_ERR_NOT_SERIALIZABLE", "NotSerializable" },
                { "OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR", "Deserialize" },
                { "OPENDAQ_ERR_CONVERSIONFAILED", "ConversionFailed" },
                { "OPENDAQ_ERR_INVALIDPROPERTY", "InvalidProperty" },
                { "OPENDAQ_ERR_INVALIDVALUE", "InvalidValue" },
                { "OPENDAQ_ERR_INVALIDSTATE", "InvalidState" },
                { "OPENDAQ_ERR_VALIDATE_FAILED", "ValidateFailed" },
                { "OPENDAQ_ERR_UNINITIALIZED", "Uninitialized" },
                { "OPENDAQ_ERR_INVALID_OPERATION", "InvalidOperation" },
                { "OPENDAQ_ERR_SIZETOOLARGE", "SizeTooLarge" },
                { "OPENDAQ_ERR_SIZETOOSMALL", "SizeTooSmall" },
                { "OPENDAQ_ERR_BUFFERFULL", "BufferFull" },
                { "OPENDAQ_ERR_DUPLICATEITEM", "DuplicateItem" },
                { "OPENDAQ_ERR_EMPTY_SCALING_TABLE", "EmptyScalingTable" },
                { "OPENDAQ_ERR_EMPTY_RANGE", "EmptyRange" },
            };

            _pointerArrays = new HashSet<string>();
            _boxedOrdinalTypes = new HashSet<string>
            {
                "Integer",
                "Float",
                "Boolean",
            };

            _interfaceToValueType = new Dictionary<string, string>
            {
                {"Integer", "RtInt"},
                {"Float", "RtFloat"},
                {"Boolean", "Boolean"},
            };

            _generationType = GenerationType.Normal;
        }

        public override void GenerateFile(string templatePath)
        {
            Initialize();

            IAttributeInfo info = RtFile.AttributeInfo;
            info.NamespaceSeparator = ".";
            info.ParameterSeparator = ";";
            // info.DefaultBasePtr = "ObjectPtrBase";

            if (info.PtrSuffix == null)
            {
                info.PtrSuffix = "Ptr";
            }

            info.PtrMappings.Add("IListObject", new SmartPtr("IListPtr", true));
            // info.PtrMappings.Add("IListPtr", new SmartPtr("IListObject", false));
            info.PtrMappings.Add("IDictObject", new SmartPtr("IDictionaryPtr", true));
            // info.PtrMappings.Add("IDictionaryPtr", new SmartPtr("IDictObject", false));
            info.PtrMappings.Add("IBaseObject", new SmartPtr("ISmartPtr", false));
            info.PtrMappings.Add("IIntObject", new SmartPtr("IIntegerPtr", false));
            info.PtrMappings.Add("IFloatObject", new SmartPtr("IFloatPtr", false));
            info.PtrMappings.Add("IBoolObject", new SmartPtr("IBoolPtr", false));
            info.PtrMappings.Add("IProcObject", new SmartPtr("IProcedurePtr", false));
            info.PtrMappings.Add("IProcedure", new SmartPtr("IProcedurePtr", false));
            info.PtrMappings.Add("IFuncObject", new SmartPtr("IFunctionPtr", false));
            info.PtrMappings.Add("IFunction", new SmartPtr("IFunctionPtr", false));

            base.GenerateFile(templatePath);
        }

        private void Initialize()
        {
            _isGeneratingInterfaceMethods = false;

            _wrappedMethods = new StringBuilder();
            _wrappedInterfaceMethods = new StringBuilder();
            _interfaceMappings = new StringBuilder();
            _interfaceMethods = new StringBuilder();
            _methodImpl = new StringBuilder();
            _interfaceMethodImpl = new StringBuilder();
        }

        protected override void SetCustomVariables()
        {
            GenerateUsings(RtFile.CurrentClass);

            AddSmartPtrOverloads(RtFile.CurrentClass);
            AddNativeOverloads(RtFile.CurrentClass);
        }

        protected override void OnVariablesReady()
        {
            GenerateInterfaceMethodDeclarations(RtFile.CurrentClass);

            TrimAndAdjustVariables();

            // IList<IRTFactory> factories = AddNativeFactoryOverloads(GroupFactoriesWithTheSameArguments());
            IList<IRTFactory> factories = AddNativeFactoryOverloads(new List<IRTFactory>(RtFile.Factories));

            string factoryConstructorDecl = GenerateFactoryConstructors(factories, false);
            if (!string.IsNullOrEmpty(factoryConstructorDecl))
            {
                _wrappedMethods.Insert(0, Environment.NewLine);
            }
            
            string factoryConstructorImpl = GenerateFactoryConstructors(factories, true);
            if (!string.IsNullOrEmpty(factoryConstructorDecl))
            {
                if (_methodImpl.Length > 0)
                {
                    _methodImpl.Insert(0, Environment.NewLine);
                }
            }

            Variables.Add("FactoryConstructors", factoryConstructorDecl);
            Variables.Add("FactoryConstructorsImpl", factoryConstructorImpl);

            Variables.Add("WrappedMethods", _wrappedMethods.ToString());
            Variables.Add("WrappedInterfaceMethods", _wrappedInterfaceMethods.ToString());

            Variables.Add("InterfaceMappings", _interfaceMappings.ToString());
            Variables.Add("InterfaceMethods", _interfaceMethods.ToString());
            Variables.Add("MethodImplementations", _methodImpl.ToString());
            Variables.Add("InterfaceMethodImplementations", _interfaceMethodImpl.ToString());
        }

        private void TrimAndAdjustVariables()
        {
            _wrappedMethods.TrimTrailingNewLines();
            _wrappedInterfaceMethods.TrimTrailingNewLines();
            _interfaceMappings.TrimTrailingNewLines();
            _interfaceMethods.TrimTrailingNewLines();
            _methodImpl.TrimTrailingNewLines();
            _interfaceMethodImpl.TrimTrailingNewLines();

            if (_interfaceMappings.Length > 0)
            {
                _interfaceMappings.Insert(0, Environment.NewLine);
            }

            if (_interfaceMethods.Length > 0)
            {
                _interfaceMethods.Insert(0, Environment.NewLine + Environment.NewLine);
            }

            if (_wrappedMethods.Length > 0)
            {
                _wrappedMethods.Insert(0, Environment.NewLine);
            }

            if (_interfaceMethodImpl.Length > 0)
            {
                _interfaceMethodImpl.Insert(0, Environment.NewLine);
            }
        }

        public bool HasValueTypeArguments(IMethod m)
        {
            return m != null && HasValueTypeArguments(m.Arguments);
        }

        public bool HasValueTypeArguments(IList<IArgument> args, bool stringOnly = false)
        {
            return args != null
                   && args.Any(argument =>
                   {
                       bool needsHelper = !argument.IsOutParam;
                       if (needsHelper)
                       {
                           needsHelper = argument.Type.Name == "IString";
                           if (!stringOnly)
                           {
                               needsHelper |= _boxedOrdinalTypes.Contains(argument.Type.NonInterfaceName);
                           }
                       }
                       return needsHelper;
                   });
        }

        private IList<IRTFactory> AddNativeFactoryOverloads(IList<IRTFactory> rtFactories)
        {
            List<IRTFactory> newFactories = new List<IRTFactory>();

            foreach (IRTFactory factory in rtFactories)
            {
                if (!HasValueTypeArguments(factory.Arguments))
                {
                    continue;
                }

                IRTFactory newFactory = factory.Clone();
                foreach (IArgument argument in newFactory.Arguments)
                {
                    ITypeName typeName = argument.Type;
                    if (typeName.Name == "IString" && !argument.IsOutParam)
                    {
                        typeName.Name = "string";
                        typeName.Modifiers = "";
                    }
                }
                
                newFactory.Tag = "_string";
                newFactories.Add(newFactory);
            }

            foreach (IRTFactory factory in newFactories)
            {
                rtFactories.Add(factory);
            }

            return rtFactories;
        }

        private void AddNativeOverloads(IRTInterface rtClass)
        {
            foreach (IMethod method in rtClass.Methods)
            {
                if (!HasValueTypeArguments(method))
                {
                    continue;
                }

                bool hasBaseObjectParam = false;
                bool hasStringParam = false;
                HashSet<string> boxedOrdinalTypes = new HashSet<string>();
                List<IArgument> arguments = new List<IArgument>();

                foreach (IArgument argument in method.Arguments)
                {
                    ITypeName typeName = argument.Type.Clone();
                    if (typeName.Name == "IString" && !argument.IsOutParam)
                    {
                        hasStringParam = true;

                        typeName.Name = "string";
                        typeName.Modifiers = "";
                    }
                    else if (!typeName.Flags.IsValueType && !argument.IsOutParam)
                    {
                        if (_boxedOrdinalTypes.Contains(typeName.NonInterfaceName))
                        {
                            boxedOrdinalTypes.Add(typeName.NonInterfaceName);
                        }

                        string ptrName = GetWrappedMethodReturnType(typeName, false);
                        if (!string.IsNullOrEmpty(ptrName))
                        {
                            typeName.Name = ptrName;

                            if (ptrName == "ISmartPtr")
                            {
                                hasBaseObjectParam = true;
                            }
                        }
                    }

                    arguments.Add(new Argument(typeName, argument.Name));
                }

                if (hasStringParam)
                {
                    method.Overloads.Add(new Overload(method, OverloadType.Helper, arguments, "string"));
                }

                if (hasBaseObjectParam)
                {
                    method.Overloads.Add(new Overload(method, OverloadType.Helper, ToProxyArgs(arguments), "string-proxy"));
                }

                if (boxedOrdinalTypes.Count > 0)
                {
                    AddValueTypeOverloads(method, boxedOrdinalTypes);
                }
            }
        }

        private void AddValueTypeOverloads(IMethod method, IEnumerable<string> boxedOrdinalTypes)
        {
            foreach (string ordinalType in boxedOrdinalTypes)
            {
                List<IArgument> args = new List<IArgument>(method.Arguments.Count);
                foreach (IArgument argument in method.Arguments)
                {
                    if (argument.Type.NonInterfaceName == ordinalType)
                    {
                        ITypeName typeName = argument.Type.Clone();
                        typeName.Name = _interfaceToValueType.TryGetValue(ordinalType, out string valueType)
                            ? valueType
                            : ordinalType;
            
                        typeName.Modifiers = "";
                        typeName.Flags.Add("BoxValueType", true);

                        args.Add(new Argument(typeName, argument.Name));
                    }
                    else
                    {
                        args.Add(argument);
                    }
                }
            
                method.Overloads.Add(new Overload(method, OverloadType.Helper, args, "BoxValueType"));
            }
        }

        private IList<IArgument> ToProxyArgs(ICollection<IArgument> arguments)
        {
            List<IArgument> args = new List<IArgument>(arguments.Count);
            foreach (IArgument argument in arguments)
            {
                if (argument.Type.Name == "ISmartPtr")
                {
                    ITypeName typeName = argument.Type.Clone();
                    typeName.Name = "TProxyValue";
                    typeName.Modifiers = "";

                    args.Add(new Argument(typeName, argument.Name));
                }
                else
                {
                    args.Add(argument);
                }
            }

            return args;
        }

        private void AddSmartPtrOverloads(IRTInterface rtClass)
        {
            foreach (IMethod method in rtClass.Methods)
            {
                int argCount = method.Arguments.Count;
                if (argCount == 0 || (argCount == 1 && method.Arguments[0].IsOutParam))
                {
                    continue;
                }

                bool add = false;
                List<IArgument> args = new List<IArgument>(method.Arguments.Count);

                foreach (IArgument argument in method.Arguments)
                {
                    if (!argument.Type.Flags.IsValueType && !argument.IsOutParam)
                    {
                        string name = GetWrappedMethodReturnArg(argument, false);
                        if (name == argument.Type.Name)
                        {
                            args.Add(argument);
                            continue;
                        }

                        IArgument arg = argument.Clone();
                        arg.Type.Name = name;

                        args.Add(arg);
                        add = true;
                    }
                    else
                    {
                        args.Add(argument);
                    }
                }

                if (add)
                {
                    method.Overloads.Add(new Overload(method, OverloadType.Wrapper, args));
                }
            }
        }

        private string GenerateFactoryConstructors(IList<IRTFactory> factories, bool implementation)
        {
            if (factories.Count == 0)
            {
                return "";
            }

            GenerationType genType = _generationType;
            _generationType = GenerationType.FactoryConstructor;

            StringBuilder factoryCtors = new StringBuilder();

            for (var i = 0; i < factories.Count; i++)
            {
                IRTFactory factory = factories[i];
                if (factory.Options.HasFlag(FactoryOptions.NoConstructor))
                {
                    continue;
                }

                if (factory.IsGeneric)
                {
                    GenerationType backup = _generationType;
                    _generationType = GenerationType.Factory;

                    string factoryConstructor = GenerateFactoryConstructor(factories, i, implementation);
                    factoryCtors.Append(factoryConstructor);

                    if (implementation)
                    {
                        factoryCtors.AppendLine();
                    }

                    _generationType = backup;
                }
                else
                {
                    string ctor = GenerateFactoryConstructor(factories, i, implementation);
                    if (implementation)
                    {
                        factoryCtors.AppendLine(ctor);
                    }
                    else
                    {
                        factoryCtors.Append(ctor);
                    }
                }
            }

            if (!implementation && factoryCtors.Length != 0)
            {
                factoryCtors.Insert(0, Environment.NewLine
                                       + "    // Factory constructors"
                                       + Environment.NewLine);
            }

            _generationType = genType;

            factoryCtors.TrimTrailingNewLine();
            return factoryCtors.ToString();
        }

        private string GenerateFactoryConstructor(IList<IRTFactory> factories, int index, bool implementation)
        {
            string templateName;
            if (implementation)
            {
                IRTFactory factory = factories[index];
                if (factory.IsConstructorWithMultipleFactories)
                {
                    templateName = "delphi.factory.ctor.tag.impl.template";
                }
                else if (factory.IsGeneric)
                {
                    templateName = "delphi.factory.ctor.impl.templated.template";
                }
                else
                {
                    templateName = "delphi.factory.ctor.impl.template";
                }
            }
            else
            {
                templateName = "delphi.factory.ctor.template";
            }

            string ctor = RenderFileTemplate(new RTFactories(factories, index), Utility.GetTemplate(templateName), (fact, variable) =>
            {
                string customValue = GetFactoryConstructorVariable(fact, variable);

                if (customValue != null)
                {
                    return customValue;
                }

                return Variables.TryGetValue(variable, out string variableValue)
                           ? variableValue
                           : null;
            });
            return ctor;
        }

        private string GetFactoryConstructorVariable(RTFactories constructors, string variable)
        {
            IRTFactory factory = constructors.Factories[constructors.Index];

            switch (variable)
            {
                case "Arguments":
                {
                    return RenderDelphiArguments(factory.ToOverload(true));
                }
                case "ArgumentNames":
                {
                    string argNames = GetArgumentNames(factory.ToOverload(true), ", ", "");
                    return !string.IsNullOrEmpty(argNames)
                               ? $", {argNames}"
                               : argNames;
                }
                case "FactoryName":
                {
                    if (IsFactoryForTheSameInterface(factory))
                    {
                        return "";
                    }

                    string ctorName = factory.PrettyName.Replace(factory.InterfaceName.Substring(1, factory.InterfaceName.Length - 1), "");
                    return ctorName;

                }
                case "FactoryDelphiName":
                {
                    return factory.Name.Capitalize();
                }
                case "UnitSpecifier":
                    return $"{GetFileVariable(RtFile.CurrentClass, "InterfaceNamespace")}{GetFileVariable(RtFile.CurrentClass, "UnitName")}.";
                case "EnumKinds":
                    return GetConstructorSwitchCases(factory);
                case "Overload":
                    if (IsFactoryForTheSameInterface(factory)
                        || constructors.Factories.Count(f => f.PrettyName == factory.PrettyName) > 1)
                    {
                        return " overload;";
                    }
                    else
                    {
                        return ";";
                    }

                case "Override":
                    if (!IsFactoryForTheSameInterface(factory))
                    {
                        return "";
                    }

                    if (factory.Arguments.Length == 0 ||
                        (factory.Arguments.Length == 1 && factory.Arguments[0].Type.Name == factory.InterfaceName))
                    {
                        return " override;";
                    }

                    return "";
                case "InterfaceTemplatesVars":
                    return GenerateSampleTypeInterfaceVars((IRTGenericFactory) factory);
                case "InterfaceTemplateFactories":
                    return GenerateTemplateInterfaceFactoryCalls((IRTGenericFactory)factory);
                case "InterfaceTemplate":
                case "PtrTemplateImplementation":
                    return GetFileVariable(RtFile.CurrentClass, variable);
                case "FactoryTemplateArgs":
                    return factory.IsGeneric
                               ? GetTemplateFactoryVariable((IRTGenericFactory) factory, variable)
                               : "";
                default:
                    return null;
            }
        }

        private bool IsFactoryForTheSameInterface(IRTFactory factory)
        {
            return RtFile.CurrentClass.Type.NonInterfaceName == factory.PrettyName ||
                   RtFile.CurrentClass.BaseType.NonInterfaceName == factory.PrettyName;
        }

        private string GenerateTemplateInterfaceFactoryCalls(IRTGenericFactory genericFactory)
        {
            string templatePath;

            switch (genericFactory.GenericTypeArguments.Rank)
            {
                case 1:
                    templatePath = Utility.GetTemplate("delphi.factory.ctor.ifs.template");
                    break;
                case 2:
                    templatePath = Utility.GetTemplate("delphi.factory.ctor.ifs.2.template");
                    break;
                default:
                    throw new NotImplementedException(
                        $"Rank {genericFactory.GenericTypeArguments.Rank} not implemented for factory constructors.");
            }

            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < genericFactory.SpecializationCount; i++)
            {
                sb.AppendLine(RenderFileTemplate(genericFactory.Specialize(i), templatePath, (factory, variable) =>
                {
                    switch (variable)
                    {
                        case "SampleType1":
                            return factory.GenericTypes[0].Name;
                        case "SampleType2":
                            return factory.GenericTypes[1].Name;
                        case "OutputArgName":
                            return $"{factory.PrettyName}_{factory.Tag.Capitalize()}";
                        case "ArgNames":
                        {
                            var genType = _generationType;

                            _generationType = GenerationType.FactoryConstructorTemplate;
                            string argNames = GetArgumentNames(factory.ToOverload(true), ", ", "");

                            _generationType = genType;

                            return argNames;
                        }
                        default:
                            return GetFactoryImplVariable(factory, variable);
                    }
                }));
            }

            sb.TrimTrailingNewLines();
            return sb.ToString();
        }

        private string GenerateSampleTypeInterfaceVars(IRTGenericFactory genericFactory)
        {
            StringBuilder sb = new StringBuilder();

            for (int i = 0; i < genericFactory.SpecializationCount; i++)
            {
                IRTFactory factory = genericFactory.Specialize(i);

                sb.AppendLine($"  {factory.PrettyName}_{factory.Tag.Capitalize()}: {factory.Arguments[0].Type.GenericName()};");
            }

            return sb.ToString();
        }

        private string GetConstructorSwitchCases(IRTFactory factory)
        {
            StringBuilder sb = new StringBuilder();

            RTConstructor ctor = (RTConstructor) factory;
            foreach (FactoryWithTag factoryWithTag in ctor.Factories)
            {
                string arguments = null;
                if (factory.Arguments.Length > 1)
                {
                    arguments = $", {GetArgumentNames(ctor.ToWrappedOverload(), ", ", "")}";
                }

                sb.AppendLine($"    {ctor.EnumPrefix}{factoryWithTag.Tag}: Err := {factoryWithTag.Name}(RawInterface{arguments});");
            }

            return sb.ToString();
        }

        protected override bool HandleMemberMethod(IRTInterface rtClass, IMethod method)
        {
            if (!_isGeneratingInterfaceMethods)
            {
                IOverload rtOverload = method.Overloads[0];

                GenerateWrappedMethodDeclaration(method);
                GenerateExplicitInterfaceImplementationMapping(rtClass, rtOverload);

                GenerateMethodImplementation(rtClass, method);
                GenerateDecoratedMethodImplementations(rtClass, rtOverload);
            }
            return true;
        }

        private void GenerateDecoratedMethodImplementations(IRTInterface rtClass, IOverload method)
        {
            // Interface method implementations
            _isGeneratingInterfaceMethods = true;
            {
                string templatePath = Utility.GetTemplate("delphi.ptr.intf.impl.template");
                string renderedTemplate = RenderFileTemplate(
                    method,
                    templatePath,
                    (rtMethod, variable) => GetMethodImplVariable(rtClass, method, variable)
                );
                _interfaceMethodImpl.AppendLine(renderedTemplate);
            }
            _isGeneratingInterfaceMethods = false;
        }

        private string RenderDelphiArguments(IOverload overload)
        {
            return GetMethodArguments(overload,
                                      "out $ArgName$: $ArgTypeName$",
                                      "$ArgName$: $ArgTypeName$",
                                      "; "
            );
        }

        protected override string GetMethodWrapperVariable(IOverload overload, string variable)
        {
            switch (variable)
            {
                case "Arguments":
                    return RenderDelphiArguments(overload);
                case "Name":
                    return GetMethodVariable(overload.Method, variable).Capitalize();
                case "ReturnTypePtr":
                    return GetWrappedMethodReturnArg(overload.GetLastByRefArgument());
                case "Overload":
                    return overload.Method.Overloads.Count > 1
                               ? " overload;"
                               : "";
                case "DocComment":
                    return overload.Method.Documentation != null
                               ? GenerateWrapperDoc(overload) + Environment.NewLine + IDENT
                               : "";
                default:
                    return base.GetMethodWrapperVariable(overload, variable);
            }
        }

        private void GenerateWrappedMethodDeclaration(IMethod method)
        {
            bool isFunction = method.ReturnsByRef();

            string templatePath = isFunction
                                      ? Utility.GetTemplate("delphi.ptr.function.template")
                                      : Utility.GetTemplate("delphi.ptr.procedure.template");

            string template = File.ReadAllText(templatePath);

            bool generateWrapper = Options.GenerateWrapper;
            GenerationType genType = _generationType;

            Options.GenerateWrapper = true;
            {
                _generationType = GenerationType.InterfaceImpl;
                WriteMethodWrapper(method, template, _wrappedMethods);

                _generationType = GenerationType.InterfacePtr;
                WriteMethodWrapper(method, template, _wrappedInterfaceMethods);
            }

            _generationType = genType;
            Options.GenerateWrapper = generateWrapper;
        }

        private void GenerateExplicitInterfaceImplementationMapping(IRTInterface rtClass, IOverload overload)
        {
            // Interface mappings
            string mappingsPath = Utility.GetTemplate("delphi.ptr.mappings.template");
            string renderFileTemplate = RenderFileTemplate(
                overload,
                mappingsPath,
                (rtMethod, variable) => GetMappingVariable(rtClass, rtMethod, variable)
            );

            _interfaceMappings.Append(renderFileTemplate);
        }

        private void GenerateInterfaceMethodDeclarations(IRTInterface rtClass)
        {
            // Interface method declarations
            _isGeneratingInterfaceMethods = true;
            {
                _interfaceMethods = WriteMethods(rtClass, "delphi.template");

                // LogToFile("log.txt", _interfaceMethods);
            }
            _isGeneratingInterfaceMethods = false;
        }

        private void LogToFile(string logFile, StringBuilder sb)
        {
            if (!Debugger.IsAttached)
            {
                return;
            }

            using (StreamWriter sw = new StreamWriter(File.Open(logFile, FileMode.Append, FileAccess.Write, FileShare.Read)))
            {
                sw.WriteLine(sb.ToString());
                sw.WriteLine();
            }
        }

        private void GenerateMethodImplementation(IRTInterface rtClass, IMethod method)
        {
            foreach (IOverload overload in method.Overloads)
            {
                string templatePath = method.ReturnsByRef()
                                          ? Utility.GetTemplate("delphi.ptr.function.impl.template")
                                          : Utility.GetTemplate("delphi.ptr.procedure.impl.template");

                bool generateWrappers = Options.GenerateWrapper;
                Options.GenerateWrapper = true;

                string renderedTemplate = RenderFileTemplate(
                    overload,
                    templatePath,
                    (rtOverload, variable) => GetMethodImplVariable(rtClass, rtOverload, variable)
                );

                Options.GenerateWrapper = generateWrappers;

                _methodImpl.AppendLine(renderedTemplate);
            }
        }

        protected override string GetMethodVariable(IMethod method, string variable)
        {
            if (variable == "Name" || variable == "UppercaseName")
            {
                string methodName = method.Name;
                if (_reservedKeywords.TryGetValue(methodName, out string nonReservedName))
                {
                    methodName = nonReservedName;
                }

                if (_isGeneratingInterfaceMethods && variable != "UppercaseName")
                {
                    return "Interface_" + methodName.Capitalize();
                }

                return methodName.Capitalize();
            }

            return base.GetMethodVariable(method, variable);
        }

        private string GetMappingVariable(IRTInterface rtClass, IOverload overload, string variable)
        {
            switch (variable)
            {
                case "Interface":
                    return rtClass.Type.GenericName();
                case "UppercaseName":
                    return GetMethodVariable(overload.Method, variable);
                default:
                    return GetMethodWrapperVariableInternal(overload, variable);
            }
        }

        private string GetMethodImplVariable(IRTInterface rtClass, IOverload overload, string variable)
        {
            IArgument arg;

            switch (variable)
            {
                case "Name":
                    return GetMethodVariable(overload.Method, _isGeneratingInterfaceMethods ? "UppercaseName" : variable)?.Capitalize();
                case "PtrCreateStart":
                    arg = overload.GetLastByRefArgument();
                    if (arg != null && !arg.Type.Flags.IsValueType && arg.Type.Name != "Pointer")
                    {
                        string ptrType = GetWrappedMethodReturnType(arg.Type).Substring(1);
                        return $"T{ptrType}.Create(";
                    }

                    return string.Empty;
                case "PtrCreateEnd":
                    arg = overload.GetLastByRefArgument();
                    if (arg != null && !arg.Type.Flags.IsValueType && arg.Type.Name != "Pointer")
                    {
                        return ")";
                    }

                    return string.Empty;
                case "NonInterfaceType":
                    return rtClass.Type.NonInterfaceName;
                case "UppercaseName":
                    return GetMethodVariable(overload.Method, variable);
                case "Arguments":
                    return RenderDelphiArguments(overload);
                case "ArgumentNames":
                    return GetArgumentNames(overload, ", ", "");
                case "ReturnTypePtr":
                    return GetWrappedMethodReturnArg(overload.GetLastByRefArgument());
                case "ReturnTypeName":
                    arg = overload.GetLastByRefArgument();
                    if (arg == null)
                    {
                        return string.Empty;
                    }

                    return !arg.Type.Flags.IsValueType
                               ? $"{arg.Type.Name}Ptr"
                               : arg.Type.Name;

                case "ReturnArgTypeName":
                    arg = overload.GetLastByRefArgument();
                    if (arg == null)
                    {
                        return string.Empty;
                    }

                    return arg.IsOutPointer
                               ? GetWrappedMethodReturnArg(arg)
                               : arg.Type.Name;
                case "InterfaceArgs":
                    if ((overload.Type != OverloadType.Wrapper && overload.Type != OverloadType.Helper))
                    {
                        return "";
                    }
                    
                    string[] args = overload.Arguments.Select(GetInterfaceArgs)
                                            .Where(s => s != null)
                                            .ToArray();
                    
                    if (args.Length != 0)
                    {
                        string separator = Environment.NewLine + "  ";
                        return separator + string.Join(separator, args);
                    }

                    return "";
                case "PtrNullChecks":
                    if (overload.Type == OverloadType.Wrapper || overload.Type == OverloadType.Helper)
                    {
                        return GeneratePtrNullChecks(overload);
                    }

                    return "";
                case "PtrTemplateImplementation":
                    return GetFileVariable(rtClass, "PtrTemplateImplementation");
                default:
                    return GetMethodWrapperVariableInternal(overload, variable);
            }
        }

        private string GetInterfaceArgs(IArgument arg)
        {
            if (arg.IsOutParam || arg.Type.Flags.IsValueType)
            {
                return null;
            }

            string intfType = arg.Type.Name;
            switch (intfType)
            {
                case "IDictionaryPtr":
                    intfType = "IDictObjectPtr";
                    break;
                case "IListPtr":
                    intfType = "IListObjectPtr";
                    break;
            }

            if (intfType != "ISmartPtr")
            {
                int index = intfType.IndexOf('<');
                if (index == -1)
                {
                    index = intfType.Length;
                }
                else if (TryGetPtrMapping(intfType.Remove(index), out string mapping))
                {
                    intfType = mapping;
                    index = -1;
                }

                if (index != -1)
                {
                    intfType = intfType.Remove(index - 3);
                }
            }
            else
            {
                intfType = "IBaseObject";
            }

            if (arg.Type.HasGenericArguments && RtFile.CurrentClass.Type.HasGenericArguments)
            {
                intfType += $"<{string.Join(", ", arg.Type.GenericArguments.Select(t => t.Name))}>";
            }

            return $"{arg.Name.Capitalize()}Intf: {intfType};";
        }

        private bool TryGetPtrMapping(string intfType, out string mapping)
        {
            foreach (var ptrMapping in RtFile.AttributeInfo.PtrMappings)
            {
                if (ptrMapping.Value.Name == intfType)
                {
                    mapping = ptrMapping.Key;
                    return true;
                }
            }

            mapping = null;
            return false;
        }

        private string GeneratePtrNullChecks(IOverload overload)
        {
            StringBuilder nullChecks = new StringBuilder();
            nullChecks.AppendLine();
            foreach (IArgument argument in overload.Arguments)
            {
                if (argument.IsOutParam || argument.Type.Flags.IsValueType)
                {
                    continue;
                }

                nullChecks.AppendLine();
                nullChecks.Append(RenderFileTemplate(argument,
                                                     Utility.GetTemplate("delphi.ptr.null.check.template"),
                                                     GetPtrNullCheckVariable)
                );
            }

            return nullChecks.Length > 2
                       ? nullChecks.ToString(0, nullChecks.Length - 2)
                       : "";
        }

        private string GetPtrNullCheckVariable(IArgument argument, string variable)
        {
            switch (variable)
            {
                case "GetInterface":
                    return argument.Type.Name == "ISmartPtr"
                               ? "GetObject"
                               : "GetInterface";
                case "ArgName2":
                    return argument.Name.Capitalize();
                case "ArgName":
                    return GetMethodArgumentVariable(argument, null, variable);
                default:
                    return "";
            }
        }

        private string GetWrappedMethodReturnArg(IArgument arg, bool generic = true)
        {
            if (arg == null)
            {
                return "";
            }

            string returnTypeName = GetWrappedMethodReturnType(arg.Type, generic);

            if (arg.IsOutPointer)
            {
                string returnType = returnTypeName.TrimEnd('*');

                return arg.Type.IsGenericArgument
                       && (_isGeneratingInterfaceMethods || _generationType == GenerationType.InterfacePtr)
                    ? $"PointerTo<{returnType}>.PT"
                    : "P" + returnType;
            }

            return returnTypeName;
        }

        private string GetWrappedMethodReturnType(ITypeName type, bool generic = true)
        {
            if (type == null)
            {
                return string.Empty;
            }

            // Ignore Event Sender and Args types for now
            if (type.NonInterfaceName == "Event" && type.UnmappedNamespace?.Raw == RtFile.AttributeInfo.CoreNamespace?.Raw)
            {
                generic = false;
            }

            if (RtFile.TypeAliases.TryGetValue(type.UnmappedName, out ITypeName alias))
            {
                return GetWrappedMethodReturnType(alias, generic);
            }

            if (type.Name == "Pointer")
            {
                return "Pointer";
            }

            if (type.Flags.IsValueType)
            {
                return type.ReturnType(false);
            }

            string templateParams = null;
            if (generic && type.GenericArguments != null)
            {
                bool emptyTemplateParam = false;
                bool unnecessarySpecialization = false;

                if (type.GenericArguments.Count == 1)
                {
                    if (string.IsNullOrEmpty(type.GenericArguments[0].Name))
                    {
                        emptyTemplateParam = true;
                    }
                    else
                    {
                        unnecessarySpecialization = type.GenericArguments[0].UnmappedName == type.UnmappedName;
                    }
                }

                if (!emptyTemplateParam && !unnecessarySpecialization)
                {
                    templateParams = $"<{string.Join(", ", type.GenericArguments.Select(param => param?.Name))}>";
                }
            }

            if (!RtFile.AttributeInfo.PtrMappings.TryGet(type.Name, out ISmartPtr ptr))
            {
                return $"{type.Name}Ptr" + templateParams;
            }

            string ptrName = ptr.Name;
            if (string.IsNullOrEmpty(ptrName))
            {
                ptrName = type.Wrapper.DefaultName;
            }

            if (ptrName.StartsWith("Generic"))
            {
                ptrName = ptrName.Substring(7);
            }

            if (!ptr.IsDefaultPtr && ptrName.EndsWith("Base"))
            {
                ptrName = ptrName.Remove(ptrName.Length - 4);
            }

            if (ptr.IsTemplated && !ptrName.EndsWith("Ptr"))
            {
                ptrName += "Ptr";
            }

            // Fix if overriding smart-pointers and if the smart-pointer starts with I (e.g. InputPort)
            if (char.ToUpperInvariant(ptrName[0]) != 'I' || ptrName.Length > 1 && char.IsLower(ptrName[1]))
            {
                ptrName = $"I{ptrName}";
            }

            if (Options.GenerateWrapper && ptrName == "ISmartPtr")
            {
                return "TProxyValue";
            }

            return ptrName + templateParams;
        }

        protected override string GetArgumentName(IOverload overload, IArgument argument)
        {
            string argName = _reservedKeywords.TryGetValue(argument.Name, out string unreservedName)
                                 ? unreservedName
                                 : argument.Name.Capitalize();

            ITypeName argumentType = argument.Type;
            if (_generationType == GenerationType.FactoryConstructorTemplate)
            { 
                if (argumentType.HasGenericArguments || argumentType.Flags.IsSpecialization)
                {
                    return argumentType.Flags.IsValueType
                               ? $"P{argumentType.Name}(@{argName})^"
                               : $"{argumentType.GenericName()}({argName})";
                }

                if (argumentType.IsGenericArgument)
                {
                    IList<ITypeName> specializedArgs = (IList<ITypeName>) overload.Tag;
                    switch (argumentType.UnmappedName)
                    {
                        case "T":
                            return $"{specializedArgs[0].Name}({argName})";
                        case "U":
                            return $"{specializedArgs[1].Name}({argName})";
                        default:
                            throw new NotImplementedException($"Generic type argument {argumentType.UnmappedName} not supported.");
                    }
                }

                if (argumentType.Name == "string")
                {
                    return $"CreateStringFromDelphiString({argName})";
                }
            }

            if ((overload.Type == OverloadType.Wrapper || overload.Type == OverloadType.Helper)
                && !argument.IsOutParam && !argumentType.Flags.IsValueType)
            {
                argName = $"{argument.Name.Capitalize()}Intf";
            }
            else if ((overload.Type == OverloadType.Helper || overload.Type == OverloadType.Constructor)
                     && argumentType.Name == "string"
                     && ((string) overload.Tag).StartsWith("string", StringComparison.Ordinal))
            {
                argName = $"CreateStringFromDelphiString({argName})";
            }
            else if (overload.Type == OverloadType.Helper
                     && argumentType.Flags.TryGet("BoxValueType", out bool isBoxed)
                     && isBoxed)
            {
                argName = $"DaqBoxValue({argName})";
            }

            return argName;
        }

        protected override string GetMethodArgumentVariable(IArgument arg, IOverload overload, string variable)
        {
            switch (variable)
            {
                case "ArgName":
                {
                    string argName = arg.Name.Capitalize();
                    return _reservedKeywords.TryGetValue(argName, out string unreserved)
                               ? unreserved
                               : argName;
                }
                case "ArgTypeName":
                    if (arg.Type != null)
                    {
                        if (arg.Type.Name == "IBaseObject" && Options.GenerateWrapper)
                        {
                            return "TProxyValue";
                        }

                        if (!Options.GenerateWrapper && arg.IsOutPointer)
                        {
                            string methodArg = GetMethodArgumentVariableInternal(arg, overload, variable);

                            return arg.Type.IsGenericArgument && !_isGeneratingInterfaceMethods
                                       ? $"PointerTo<{methodArg}>.PT"
                                       : "P" + methodArg;
                        }

                        if (arg.Type.HasGenericArguments && RtFile.CurrentClass.Type.HasGenericArguments)
                        {
                            return arg.Type.GenericName();
                        }

                        switch (_generationType)
                        {
                            case GenerationType.Factory:
                            case GenerationType.FactoryWrapper:
                            case GenerationType.FactoryConstructor:
                            case GenerationType.InterfacePtr when overload.Type == OverloadType.Wrapper:
                            case GenerationType.InterfaceImpl when overload.Type == OverloadType.Wrapper:
                            case GenerationType.Normal when overload.Type == OverloadType.Wrapper:
                            {
                                if (arg.Type.UnmappedName != "IEventPtr")
                                {
                                    return arg.Type.GenericName();
                                }
                                goto default;
                            }
                            default:
                                return arg.Type.Name;
                        }
                    }
                    // have to explicitly fall-trough
                    goto default;
                default:
                    return base.GetMethodArgumentVariable(arg, overload, variable);
            }
        }

        public override void RegisterTypeMappings(Dictionary<string, string> mappings)
        {
            mappings.Add("IList", "IListObject");
            mappings.Add("IDict", "IDictObject");
            mappings.Add("void", "Pointer");
            mappings.Add("Bool", "Boolean");
            mappings.Add("Int", "RtInt");
            mappings.Add("UInt", "RtUInt");
            mappings.Add("uint64_t", "RtUInt");
            mappings.Add("byte", "Byte");
            mappings.Add("int8", "Int8");
            mappings.Add("int16", "Int16");
            mappings.Add("int32", "Int32");
            mappings.Add("int64", "Int64");
            mappings.Add("Float", "RtFloat");
            mappings.Add("double", "RtFloat");
            mappings.Add("float", "Single");
            mappings.Add("IFuncObject", "IFunction");
            mappings.Add("FuncObject", "Function");
            mappings.Add("IProcObject", "IProcedure");
            mappings.Add("Procedure", "Procedure");
            mappings.Add("CoreType", "TCoreType");
            mappings.Add("IntfID", "TGUID");

            foreach (IEnumeration enumeration in RtFile.Enums)
            {
                mappings.Add(enumeration.Name, $"T{enumeration.Name}");
            }
        }

        private string GetUnitUsesFromType(ITypeName type, IRTInterface rtClass)
        {
            if (type.Name == "Pointer")
            {
                return null;
            }

            if (type.NonInterfaceName == "BaseObject")
            {
                return $"{type.Namespace.ToString(".")}.ProxyValue";
            }

            if (type.Flags.IsValueType)
            {
                return null;
            }

            if (type.FullName(false) == rtClass.Type.FullName(false))
            {
                return null;
            }

            // Exclude enums defined in this file from the usings list
            if (RtFile.Enums.Any(e => e.Name == type.UnmappedName))
            {
                return null;
            }

            string baseUnit = type.Namespace?.ToString(".");
            if (string.IsNullOrEmpty(baseUnit))
            {
                baseUnit = rtClass.Type.Namespace?.ToString(".");
            }

            if (!string.IsNullOrEmpty(baseUnit))
            {
                string typeName = GetUnreservedUnitName(type);


                string usingName = $"{baseUnit}.{typeName}";
                return usingName.StartsWith("daq.")
                           ? usingName.Replace("daq.", "DAQ.")
                           : usingName;
            }

            return null;
        }

        private static string GetTemplateArguments(ITypeName type, bool declaration = true, bool addEndSeparator = true)
        {
            if (type.GenericArguments == null)
            {
                return "";
            }

            string interfaceTemplateArgs = string.Join(declaration ? "; " : ", ", type.GenericArguments.Select(arg => arg.Name));
            if (addEndSeparator && !string.IsNullOrEmpty(interfaceTemplateArgs))
            {
                interfaceTemplateArgs += declaration ? "; " : ", ";
            }

            return interfaceTemplateArgs;
        }

        protected override string GetFileVariable(IRTInterface rtClass, string variable)
        {
            string interfaceArg;
            string templateArguments;

            switch (variable)
            {
                case "PtrRegistrations":
                {
                    if (rtClass.Type.HasGenericArguments)
                    {
                        return string.Empty;
                    }

                    return Environment.NewLine
                           + RenderFileTemplate(rtClass,
                                                Utility.GetTemplate("delphi.ptr.registry.template"),
                                                GetFileVariable);
                }
                case "PtrAlias":
                {
                    string templatePath;
                    if (rtClass.Type.HasGenericArguments)
                    {
                        templatePath = Utility.GetTemplate("delphi.alias.ptr.template");
                    }
                    else if (rtClass.Type.Flags.IsGeneric)
                    {
                        templatePath = Utility.GetTemplate("delphi.alias.ptr.templated.template");
                    }
                    else
                    {
                        return "";
                    }

                    return Environment.NewLine
                           + Environment.NewLine
                           + RenderFileTemplate(rtClass,
                                                templatePath,
                                                GetFileVariable).TrimEnd();
                }
                case "InterfacePtrAliasPrototype":
                    if (rtClass.Type.Flags.IsGeneric && !rtClass.Type.HasGenericArguments)
                    {
                        return Environment.NewLine
                               + Environment.NewLine
                               + RenderFileTemplate(rtClass,
                                                    Utility.GetTemplate("delphi.alias.interface.templated.template"),
                                                    GetFileVariable).TrimEnd(new []{'\r', '\n'});
                    }

                    return "";
                case "InterfacePtrAlias" when rtClass.Type.HasGenericArguments:
                {
                    string ptrAlias = RenderFileTemplate(rtClass,
                                                         Utility.GetTemplate("delphi.alias.interface.template"),
                                                         GetFileVariable).TrimEnd();

                    if (string.IsNullOrEmpty(ptrAlias))
                    {
                        return "";
                    }

                    return Environment.NewLine
                           + Environment.NewLine
                           + ptrAlias;
                }
                case "InterfacePtrAlias":
                {
                    return "";
                }
                case "InterfaceNamespace":
                    string delphiNamespace = rtClass.Type.Namespace?.ToString(".");
                    if (!string.IsNullOrEmpty(delphiNamespace))
                    {
                        delphiNamespace += ".";
                    }

                    return delphiNamespace;
                case "CustomTypes":
                    return GenerateCustomTypes();
                case "UnitName":
                {
                    return GetUnreservedUnitName(rtClass.Type);
                }
                case "ImplUsesUnits":
                    return ";";
                case "GenericPointerTypes":
                    if (!rtClass.Type.HasGenericArguments)
                    {
                        return string.Empty;
                    }

                    string templatedTypePointers = string.Join(Environment.NewLine,
                                                               rtClass.Type.GenericArguments
                                                                      .Where(t => t.IsGenericArgument)
                                                                      .Select(t => $"    P{t.Name} = ^{t.Name};"));

                    return Environment.NewLine + $"  type{Environment.NewLine}{templatedTypePointers}";
                case "PtrTemplate":
                {
                    if (!rtClass.Type.Flags.IsGeneric)
                    {
                        return string.Empty;
                    }

                    templateArguments = GetTemplateArguments(rtClass.Type);

                    interfaceArg = null;
                    if (!string.IsNullOrEmpty(templateArguments))
                    {
                        interfaceArg = $"<{templateArguments.Remove(templateArguments.Length - 2).Replace(';', ',')}>";
                    }

                    return $"<{templateArguments}InterfaceType: {rtClass.Type.Name}{interfaceArg}>";
                }
                case "PtrTemplateOpen":
                {
                    if (!rtClass.Type.Flags.IsGeneric)
                    {
                        return string.Empty;
                    }

                    templateArguments = GetTemplateArguments(rtClass.Type, false);

                    interfaceArg = null;
                    if (!string.IsNullOrEmpty(templateArguments))
                    {
                        interfaceArg = $"<{templateArguments.Remove(templateArguments.Length - 2).Replace(';', ',')}>";
                    }

                    return $"<{templateArguments}{rtClass.Type.Name}{interfaceArg}>";
                }
                case "PtrTemplateHelper":
                case "PtrTemplateHelperPlain":
                {
                    if (!rtClass.Type.Flags.IsGeneric || rtClass.Type.GenericArguments == null)
                    {
                        return string.Empty;
                    }

                    templateArguments = GetTemplateArguments(rtClass.Type, variable == "PtrTemplateHelper", false);
                    return $"<{templateArguments}>";
                }
                case "PtrTemplateImplementation":
                {
                    if (!rtClass.Type.Flags.IsGeneric)
                    {
                        return string.Empty;
                    }

                    templateArguments = GetTemplateArguments(rtClass.Type, false);
                    return $"<{templateArguments}{GetFileVariable(rtClass, "PtrIntferfaceArg")}>";
                }
                case "PtrIntferfaceArg":
                    return rtClass.Type.Flags.IsGeneric
                               ? "InterfaceType"
                               : rtClass.Type.Name;

                case "PtrName":
                case "PtrNameOnly":
                {
                    bool hasPtrOverride = RtFile.AttributeInfo.PtrMappings.TryGet(rtClass.Type.Name, out ISmartPtr ptr);
                    if (hasPtrOverride &&
                        !ptr.IsDefaultPtr &&
                        !string.IsNullOrEmpty(ptr.Name) &&
                        ptr.Name.EndsWith("Base"))
                    {
                        return rtClass.Type.Wrapper.Name.Remove(rtClass.Type.Wrapper.Name.Length - 4);
                    }

                    if (hasPtrOverride && !string.IsNullOrEmpty(ptr.Name)
                                       && ptr.Name.StartsWith("Generic", StringComparison.Ordinal))
                    {
                        return ptr.Name.Substring(7);
                    }

                    return null;
                }
                case "PtrNameFull":
                {
                    bool hasPtrOverride = RtFile.AttributeInfo.PtrMappings.TryGet(rtClass.Type.Name, out ISmartPtr ptr);
                    if (hasPtrOverride &&
                        !ptr.IsDefaultPtr &&
                        !string.IsNullOrEmpty(ptr.Name) &&
                        ptr.Name.EndsWith("Base"))
                    {
                        return rtClass.Type.Wrapper.NameFull.Remove(rtClass.Type.Wrapper.NameFull.Length - 4);
                    }
                    
                    return null;
                }
                case "BasePtrName":
                case "BasePtrNameOnly":
                    if (rtClass.BaseType.Name == "IBaseObject")
                    {
                        return RtFile.AttributeInfo.DefaultBasePtr;
                    }

                    if (rtClass.BaseType.Name == "IConnection")
                    {
                        return "ConnectionPtr";
                    }

                    if (!string.IsNullOrEmpty(rtClass.BaseType.Wrapper.BaseName)
                        && rtClass.BaseType.Wrapper.BaseName.StartsWith("Generic"))
                    {
                        return rtClass.BaseType.Wrapper.BaseName.Substring(7);
                    }

                    return null;
                case "PtrInterfaceTemplate":
                    return rtClass.Type.Flags.IsGeneric
                               ? "<InterfaceType>"
                               : "";
                case "InterfaceGuidAttribute":
                {
                    return !rtClass.Type.HasGenericArguments
                               ? $"  ['{{{Variables["InterfaceGuid"]}}}']" + Environment.NewLine
                               : "";
                }
                case "InterfacePtrGuid":
                    return Utility.InterfaceUuid(rtClass.Type, "Ptr").ToString("D");
                case "BaseInterfacePtr":
                    if (rtClass.BaseType.Name == "IBaseObject")
                    {
                        return "IObjectPtr";
                    }

                    return GetWrappedMethodReturnType(rtClass.BaseType, false);
                case "FactoryFunctionDeclarations":
                    return RenderFactoryDeclarations();
                case "FactoryFunctionImplementations":
                    return RenderFactoryImplementations();
                case "FactoryWrapperFunctionImplementations":
                    return RenderFactoryWrapperImplementations();
                case "FactoryConfigUnit":
                    if (RtFile.Factories != null && RtFile.Factories.Count > 0)
                    {
                        string usingName = $"{rtClass.Type.Namespace.ToString(".")}.{Options.LibraryInfo.Name}.Config";

                        return $"{Environment.NewLine}  {(usingName.StartsWith("daq.") ? usingName.Replace("daq.", "DAQ.") : usingName)},";
                    }

                    return "";
                case "GenericInterfaceInstatiations":
                {
                    string interfaceSpecialization = "";
                    if (rtClass.Type.HasGenericArguments)
                    {
                        interfaceSpecialization = GenerateExplicitInterfaceTemplates(rtClass);
                    }

                    if (!string.IsNullOrEmpty(interfaceSpecialization))
                    {
                        interfaceSpecialization = Environment.NewLine + interfaceSpecialization;
                    }

                    return interfaceSpecialization;
                }
            }

            return base.GetFileVariable(rtClass, variable);
        }

        private string GetTemplateFactoryVariable(IRTGenericFactory factory, string variable)
        {
            switch (variable)
            {
                case "FactoryTemplateArgs":
                {
                    switch (factory.GenericTypeArguments.Rank)
                    {
                        case 1:
                            return "<T>";
                        case 2:
                                return "<T, U>";
                        default:
                            throw new NotImplementedException($"Unsupported generic factory \"{factory.Name}\" rank {factory.GenericTypeArguments.Rank}.");
                    }
                }
                case "ArgumentNamesFactory":
                    return GetArgumentNames(factory.ToOverload(), ", ", "");
                default:
                {
                    var genType = _generationType;

                    _generationType = GenerationType.FactoryWrapper;

                    string result = GetFactoryImplVariable(factory, variable)
                                    ?? GetFactoryImplWrapperVariable(factory, variable);

                    _generationType = genType;
                    return result;
                }
            }
        }

        private string GenerateExplicitInterfaceTemplates(IRTInterface rtClass)
        {
            IRTGenericFactory genericFactory = (IRTGenericFactory) RtFile.Factories?.FirstOrDefault(factory => factory.IsGeneric);
            if (genericFactory == null && (!rtClass.Type.HasGenericArguments || RtFile.Tag == null))
            {
                return "";
            }

            ISampleTypes specializations;
            if (genericFactory != null)
            {
                specializations = ((SampleTypes) genericFactory.GenericTypeArguments);
            }
            else if (!((Dictionary<string, ISampleTypes>)RtFile.Tag).TryGetValue("DSRT_DEFAULT_SAMPLE_TYPES", out specializations))
            {
                return "";
            }

            AddSampleTypeMappings(specializations);

            StringBuilder sb = new StringBuilder(Environment.NewLine);
            foreach (IList<ITypeName> specialization in specializations.Types)
            {
                sb.AppendLine(RenderFileTemplate(new InterfaceSpecialization(rtClass, specialization),
                                                 Utility.GetTemplate("delphi.template.specialization.template"),
                                                 GetExplicitInterfaceVariable));
            }

            sb.TrimTrailingNewLines();
            return sb.ToString();
        }

        private void AddSampleTypeMappings(ISampleTypes specializations)
        {
            foreach (ITypeName typeName in specializations.Types
                                                          .SelectMany(s => s))
            {
                if (typeName.Flags.IsValueType &&
                    !typeName.Flags.IsCoreType &&
                    !RtFile.AttributeInfo.TypeMappings.Has(typeName.UnmappedName) &&
                    !typeName.IsGenericArgument
                )
                {
                    RtFile.AttributeInfo.TypeMappings.Add(typeName.UnmappedName, $"T{typeName.UnmappedName}");
                }
            }
        }

        private string GetExplicitInterfaceVariable(Specialization specialization, string variable)
        {
            switch (variable)
            {
                case "SpecializationGuid":
                    return specialization.Inteface.Type.Guid.ToString("D");
                case "Specialization":
                    return string.Join("", specialization.Types.Select(t => t.UnmappedName.Capitalize()));
                default:
                    return GetFileVariable(specialization.Inteface, variable);
            }
        }

        private string RenderFactoryImplementations()
        {
            GenerationType prev = _generationType;
            _generationType = GenerationType.Factory;

            StringBuilder sb = new StringBuilder();
            foreach (IRTFactory factory in RtFile.Factories)
            {
                RenderFactoryImplementation(factory, sb);
            }

            _generationType = prev;

            return sb.ToString();
        }

        private void RenderFactoryImplementation(IRTFactory factory, StringBuilder sb)
        {
            if (!string.IsNullOrEmpty(factory.Tag) && factory.Tag[0] == '_')
            {
                return;
            }

            if (factory.IsGeneric)
            {
                IRTGenericFactory genericFactory = (IRTGenericFactory) factory;
                
                for (int i = 0; i < genericFactory.SpecializationCount; i++)
                {
                    RenderFactoryImplementation(genericFactory.Specialize(i), sb);
                }
            }
            else
            {
                sb.Append(RenderFileTemplate(factory,
                                             Utility.GetTemplate("delphi.factory.impl.template"),
                                             GetFactoryImplVariable));
            }
        }

        private string RenderFactoryWrapperImplementations()
        {
            StringBuilder sb = new StringBuilder();
            foreach (IRTFactory factory in RtFile.Factories)
            {
                if (!factory.Options.HasFlag(FactoryOptions.Wrap) || (!string.IsNullOrEmpty(factory.Tag) && factory.Tag[0] == '_'))
                {
                    continue;
                }

                sb.AppendLine();

                bool generateWrapper = Options.GenerateWrapper;
                Options.GenerateWrapper = true;
                {
                    sb.Append(RenderFileTemplate(factory,
                                                 Utility.GetTemplate("delphi.factory.impl.wrapper.template"),
                                                 GetFactoryImplWrapperVariable));
                }
                Options.GenerateWrapper = generateWrapper;
            }

            return sb.ToString();
        }

        private string RenderFactoryDeclarations()
        {
            StringBuilder sb = new StringBuilder();
            foreach (IRTFactory factory in RtFile.Factories)
            {
                if (factory.IsGeneric)
                {
                    IRTGenericFactory genericFactory = (IRTGenericFactory) factory;
                    for (int i = 0; i < genericFactory.SpecializationCount; i++)
                    {
                        RenderFactoryDeclaration(genericFactory.Specialize(i), sb);
                    }
                }
                else
                {
                    RenderFactoryDeclaration(factory, sb);
                }
            }

            return sb.ToString();
        }

        private void RenderFactoryDeclaration(IRTFactory factory, StringBuilder sb)
        {
            if (!string.IsNullOrEmpty(factory.Tag) && factory.Tag[0] == '_')
            {
                return;
            }

            GenerationType genType = _generationType;

            FactoryOptions options = factory.Options;
            if (!options.HasFlag(FactoryOptions.NoDeclaration))
            {
                _generationType = GenerationType.Factory;

                sb.Append(RenderFileTemplate(factory,
                                             Utility.GetTemplate("delphi.factory.template"),
                                             GetFactoryVariable));

                _generationType = genType;
            }

            if (options.HasFlag(FactoryOptions.Wrap))
            {
                _generationType = GenerationType.FactoryWrapper;

                bool generateWrapper = Options.GenerateWrapper;
                Options.GenerateWrapper = true;
                {
                    sb.Append(RenderFileTemplate(factory,
                                                 Utility.GetTemplate("delphi.factory.wrapper.template"),
                                                 GetFactoryWrapperVariable));
                }
                Options.GenerateWrapper = generateWrapper;

                _generationType = genType;
            }
        }

        private string GetFactoryWrapperVariable(IRTFactory factory, string variable)
        {
            switch (variable)
            {
                case "FactoryDelphiName":
                    return factory.PrettyName;
                case "Interface":
                    return factory.InterfaceName;
            }

            return GetFactoryVariable(factory, variable);
        }

        private string GetFactoryImplVariable(IRTFactory factory, string variable)
        {
            switch (variable)
            {
                case "FactoryName":
                    return factory.Name;
                case "FactoryDelphiName":
                    return factory.Name.Capitalize();
                case "Arguments":
                    return RenderDelphiArguments(factory.ToOverload());
                case "DLLName":
                    return Options.LibraryInfo.Name + "DLL";
                default:
                    return null;
            }
        }

        private string GetFactoryImplWrapperVariable(IRTFactory factory, string variable)
        {
            switch (variable)
            {
                case "FactoryDelphiName":
                    return factory.PrettyName;
                case "FactoryUnwrappedName":
                    return GetFactoryVariable(factory, "FactoryDelphiName");
                case "Interface":
                    return factory.InterfaceName;
                case "ArgumentNames":
                    return GetFactoryConstructorVariable(new RTFactories(new List<IRTFactory>{factory}, 0), variable);
            }

            return GetFactoryImplVariable(factory, variable);
        }

        private string GetFactoryVariable(IRTFactory factory, string variable)
        {
            switch (variable)
            {
                case "FactoryDelphiName":
                    return factory.Name.Capitalize();
                case "Arguments":
                    return RenderDelphiArguments(factory.ToOverload());
                default:
                    return null;
            }
        }

        private string GenerateDocAttributes(IOverload overload, IEnumerable<IDocTag> tags)
        {
            StringBuilder sb = new StringBuilder();

            foreach (IDocTag tag in tags)
            {
                switch (tag.TagType)
                {
                    case TagType.Brief:
                    {
                        var brief = (IDocBrief)tag;

                        int lineCount = brief.Lines.Count;
                        int skipLines = lineCount == 1 ? 1 : 0;

                        sb.Append("/// <summary>");
                        if (lineCount == 1)
                        {
                            sb.AppendLine(brief.Lines[0].FullText);
                        }
                        else
                        {
                            sb.AppendLine();
                        }

                        foreach (IDocLine docLine in brief.Lines.Skip(skipLines))
                        {
                            sb.Append(' ', 4);
                            sb.Append("/// ");
                            sb.AppendLine(docLine.FullText);
                        }

                        if (lineCount > 1)
                        {
                            sb.Append(' ', 4);
                            sb.AppendLine("/// </summary>");
                        }
                        else
                        {
                            sb.TrimTrailingNewLine();
                            sb.AppendLine("</summary>");
                        }
                        break;
                    }
                    case TagType.Param:
                    {
                        IDocParam param = (IDocParam)tag;
                        if (param.ParamName == overload.GetLastByRefArgument()?.Name)
                        {
                            sb.Append(ConvertDocAttribute("returns", overload, param));
                        }
                        else
                        {
                            int numLines = param.Lines.Count;
                            if (numLines > 0)
                            {
                                string paramLine = param.Lines[0].FullText;
                                if (paramLine.StartsWith("@param"))
                                {
                                    paramLine = paramLine.Remove(0, "@param".Length);
                                }
                                paramLine = param.Lines[0].FullText.Replace("@param", "");

                                sb.AppendLine(numLines == 1
                                    ? $"    /// <param name=\"{param.ParamName}\">{paramLine}</param>"
                                    : $"    /// <param name=\"{param.ParamName}\">{paramLine}");

                                for (int i = 1; i < numLines; ++i)
                                {
                                    sb.AppendLine($"    /// {param.Lines[i].FullText}");
                                }

                                if (numLines > 1)
                                {
                                    sb.AppendLine("    /// </param>");
                                }
                            }

                        }

                        break;
                    }
                    case TagType.ParamRef:
                    case TagType.Private:
                        return tag.RawText;
                    case TagType.RetVal:
                        var retVal = (IDocRetVal)tag;
                        if (retVal.ReturnValue.StartsWith(ERROR_CODE_PREFIX))
                        {
                            sb.Append(ConvertDocAttribute("throws", overload, retVal));
                            break;
                        }
                        else if (retVal.ReturnValue == "OPENDAQ_IGNORED")
                        {
                            sb.Append(ConvertDocAttribute("returns nullptr", overload, retVal));
                            break;
                        }
                        goto case TagType.Unknown;
                    case TagType.Description:
                    {
                        var description = (IDocDescription)tag;
                        int actualLines = description.Lines.Count;

                        if (string.IsNullOrEmpty(description.Lines[0].FullText))
                        {
                            --actualLines;
                            description.Lines.RemoveAt(0);
                        }

                        sb.Append(' ', 4);
                        if (actualLines > 1)
                        {
                            sb.AppendLine("/// <remarks>");
                            sb.Append(' ', 4);
                            sb.Append("/// ");
                        }
                        else
                        {
                            sb.Append("/// <remarks>");
                        }

                        sb.AppendLine(description.Lines[0].FullText);

                        foreach (IDocLine docLine in description.Lines.Skip(1))
                        {
                            if (string.IsNullOrEmpty(docLine.FullText))
                            {
                                --actualLines;
                                continue;
                            }

                            sb.Append(' ', 4);
                            sb.Append("/// ");
                            sb.AppendLine(docLine.FullText);
                        }

                        if (actualLines > 1)
                        {
                            sb.Append(' ', 4);
                            sb.Append("/// </remarks>");
                        }
                        else
                        {
                            sb.TrimTrailingNewLine();
                            sb.AppendLine("</remarks>");
                        }
                        break;
                    }
                    case TagType.Throws:
                        sb.AppendLine($"     /// {tag.RawText}");
                        break;
                    case TagType.Unknown:
                    {
                        string[] lines = tag.RawText.Split(new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
                        foreach (string line in lines)
                        {
                            string trimmed = line.TrimStart(' ', '*');
                            sb.AppendLine($"    /// {trimmed}");
                        }

                        break;
                    }
                }
            }

            return sb.ToString();
        }

        private string ConvertDocAttribute(string tagName, IOverload overload, IDocAttribute tag)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("    /// ");
            sb.Append($"<{tagName}>");

            if (tagName == "throws" && tag is IDocRetVal retVal)
            {
                sb.Append(ErrCodeToException(retVal.ReturnValue));
                sb.Append(" ");
            }

            if (tag.Lines.Count != 0)
            {
                sb.Append(tag.Lines[0].FullText);
                foreach (IDocLine line in tag.Lines.Skip(1))
                {
                    sb.Append("    /// ");
                    sb.Append(line.FullText);
                }
            }

            sb.AppendLine($"<{tagName}>");
            return sb.ToString();
        }

        private string ErrCodeToException(string errorCode)
        {
            if (!_knownExceptions.TryGetValue(errorCode, out string exception))
            {
                string codeName = errorCode.Remove(ERROR_CODE_PREFIX.Length).ToLowerInvariant();
                foreach (string segment in codeName.Split(new[] { '_' }, StringSplitOptions.RemoveEmptyEntries))
                {
                    exception += segment.Capitalize();
                }
            }

            return $"{exception}Exception";
        }

        private string GenerateWrapperDoc(IOverload overload)
        {
            IDocComment doc = overload.Method.Documentation;

            StringBuilder sb = new StringBuilder();

            var tags = new List<IDocTag>();
            if (doc.Brief != null)
            {
                tags.Add(doc.Brief);
            }

            tags.AddRange(doc.Tags);

            if (doc.Description != null)
            {
                tags.Add(doc.Description);
            }

            sb.Append(GenerateDocAttributes(overload, tags));

            sb.TrimTrailingNewLine();
            return sb.ToString();
        }

        private string GetUnreservedUnitName(ITypeName type)
        {
            string typeName = type.NonInterfaceName;

            // Avoid clash between IFloat and Float
            if (!type.Flags.IsValueType && typeName == "Float")
            {
                return typeName;
            }

            if (type != RtFile.CurrentClass.Type
                && RtFile.AttributeInfo.TypeMappings.TryGet(typeName, out string mappedName))
            {
                typeName = mappedName;
            }

            if (_reservedKeywords.ContainsKey(typeName))
            {
                typeName = $"T{typeName}";
            }

            return typeName;
        }

        public static string GetEnumPrefix(string enumName)
        {
            StringBuilder prefix = new StringBuilder(3);

            foreach (char c in enumName)
            {
                if (char.IsUpper(c))
                {
                    prefix.Append(char.ToLowerInvariant(c));
                }
            }

            return prefix.ToString();
        }

        private string GenerateCustomTypes()
        {
            if (RtFile.Enums.Count == 0
                && _pointerArrays.Count == 0
                && RtFile.TypeAliases.Count == 0)
            {
                return string.Empty;
            }

            StringBuilder customTypes = new StringBuilder();
            foreach (IEnumeration enumeration in RtFile.Enums)
            {
                if (enumeration.Options.Count <= 0)
                {
                    continue;
                }

                customTypes.Append("  ");
                GenerateEnum(enumeration, customTypes);
            }

            ITypeName interfaceType = RtFile.CurrentClass.Type;

            foreach (string pointerArray in _pointerArrays)
            {
                if (interfaceType.HasGenericArguments &&
                    interfaceType.GenericArguments.Any(t => t.IsGenericArgument && t.UnmappedName == pointerArray))
                {
                    continue;
                }

                customTypes.AppendLine($"  P{pointerArray} = ^{pointerArray};");
            }

            foreach (var (alias, type) in RtFile.TypeAliases)
            {
                if (!type.IsGenericArgument)
                {
                    customTypes.AppendLine($"  {alias} = {type.GenericName()};");
                }
            }

            if (customTypes.Length > 0)
            {
                customTypes.Insert(0, Environment.NewLine);
            }

            customTypes.TrimTrailingNewLines();
            return customTypes.ToString();
        }

        private void GenerateEnum(IEnumeration enumeration, StringBuilder buffer)
        {
            string enumPrefix = GetEnumPrefix(enumeration.Name);

            buffer.Append($"T{enumeration.Name} = (");
            buffer.Append(enumeration.Options.Aggregate(string.Empty,
                                                        (aggregate, enumOption) =>
                                                        {
                                                            string enumValue = enumOption.HasValue ? $"={enumOption.Value}" : string.Empty;
                                                            return aggregate + ", " + enumPrefix + enumOption.Name.Capitalize() + enumValue;
                                                        }).Substring(2));

            buffer.AppendLine(");");
        }

        private void GenerateUsings(IRTInterface rtClass)
        {
            string[] units = GetUsingsFromRtClassAndFactories(rtClass);

            string unitStr = string.Empty;
            int unitCount = units.Length;
            if (unitCount > 0)
            {
                StringBuilder result = new StringBuilder();

                for (int i = 0; i < unitCount - 1; i++)
                {
                    result.AppendLine($"  {units[i]},");
                }

                result.Append($"  {units[unitCount - 1]};");
                unitStr = result.ToString();

                Variables.Add("UnitsSeparator", ",");
            }
            else
            {
                Variables.Add("UnitsSeparator", ";");
            }

            Variables.Add("UsesUnits", unitStr);
        }

        private string[] GetUsingsFromRtClassAndFactories(IRTInterface rtClass)
        {
            // Base interface unit
            HashSet<string> units = new HashSet<string>();
            string unit = GetUnitUsesFromType(rtClass.BaseType, rtClass);

            if (!string.IsNullOrEmpty(unit))
            {
                units.Add(unit);
            }

            GenerateUsesFromAliases(units, RtFile.TypeAliases, rtClass);
            GetUnitUsesFromFactories(units, rtClass);
            GetUnitUsesFromMethodArguments(units, rtClass);

            GetUnitUsesFromExplicitIncludes(units, RtFile.AttributeInfo.AdditionalIncludes, rtClass);
            GetUnitUsesFromExplicitIncludes(units, RtFile.AttributeInfo.AdditionalPtrIncludes, rtClass);

            string[] unitsArray = new string[units.Count];
            units.CopyTo(unitsArray);

            return unitsArray;
        }

        private void GenerateUsesFromAliases(ISet<string> units, IDictionary<string, ITypeName> typeAliases, IRTInterface rtClass)
        {
            foreach (var (_, aliased) in typeAliases)
            {
                string includeUnit = GetUnitUsesFromType(aliased, rtClass);
                if (!string.IsNullOrEmpty(includeUnit))
                {
                    units.Add(includeUnit);
                }

                if (aliased.HasGenericArguments)
                {
                    GetUnitUsesFromGenericArguments(units, rtClass, aliased);
                }
            }
        }

        private void GetUnitUsesFromExplicitIncludes(HashSet<string> units, IEnumerable<ITypeName> includes, IRTInterface rtClass)
        {
            foreach (ITypeName include in includes)
            {
                string includeUnit = GetUnitUsesFromType(include, rtClass);
                if (!string.IsNullOrEmpty(includeUnit))
                {
                    units.Add(includeUnit);
                }
            }
        }

        private void GetUnitUsesFromFactories(HashSet<string> units, IRTInterface rtClass)
        {
            foreach (IRTFactory factory in RtFile.Factories)
            {
                foreach (IArgument argument in factory.Arguments)
                {
                    GetUnitUsesFromArgument(units, argument, rtClass);
                }

                if (!factory.IsGeneric)
                {
                    continue;
                }

                IRTGenericFactory genericFactory = (IRTGenericFactory) factory;
                foreach (ITypeName genericArg in genericFactory.GenericTypeArguments.Types.SelectMany(t => t))
                {
                    CheckAddTypeMapping(genericArg);

                    if (!genericArg.IsGenericArgument)
                    {
                        GetUnitUsesFromType(genericArg, rtClass);
                    }
                }
            }
        }

        private void GetUnitUsesFromMethodArguments(HashSet<string> units, IRTInterface rtClass)
        {
            foreach (IMethod method in rtClass.Methods)
            {
                foreach (IArgument argument in method.Arguments)
                {
                    GetUnitUsesFromArgument(units, argument, rtClass);
                }
            }
        }

        private void GetUnitUsesFromArgument(HashSet<string> units, IArgument argument, IRTInterface rtClass)
        {
            ITypeName argumentType = argument.Type;

            if (RtFile.TypeAliases.ContainsKey(argumentType.UnmappedName))
            {
                return;
            }

            string argUnit = GetUnitUsesFromType(argumentType, rtClass);
            if (!string.IsNullOrEmpty(argUnit))
            {
                units.Add(argUnit);
            }

            CheckAddTypeMapping(argumentType);

            if (argument.IsOutPointer)
            {
                _pointerArrays.Add(argument.Type.Name);
            }

            GetUnitUsesFromGenericArguments(units, rtClass, argumentType);
        }

        private void CheckAddTypeMapping(ITypeName argumentType)
        {
            if (argumentType.Flags.IsValueType &&
                !argumentType.Flags.IsCoreType &&
                !RtFile.AttributeInfo.TypeMappings.Has(argumentType.UnmappedName) &&
                !argumentType.IsGenericArgument
            )
            {
                RtFile.AttributeInfo.TypeMappings.Add(argumentType.UnmappedName, $"T{argumentType.UnmappedName}");
            }
        }

        private void GetUnitUsesFromGenericArguments(ISet<string> units, IRTInterface rtClass, ITypeName type)
        {
            if (!type.HasGenericArguments)
            {
                return;
            }

            // Ignore Event Sender and Args types for now
            if (type.NonInterfaceName == "Event" && type.UnmappedNamespace?.Raw == RtFile.AttributeInfo.CoreNamespace?.Raw)
            {
                return;
            }

            foreach (ITypeName templateType in type.GenericArguments)
            {
                if (templateType.IsGenericArgument)
                {
                    continue;
                }

                CheckAddTypeMapping(templateType);

                string templateUnit = GetUnitUsesFromType(templateType, rtClass);
                if (!string.IsNullOrEmpty(templateUnit))
                {
                    units.Add(templateUnit);
                }
            }
        }
    }
}
