using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using RTGen.Generation;
using RTGen.Interfaces;
using RTGen.Types;
using RTGen.Util;

namespace RTGen.Cpp.Generators
{
    sealed class PropertyClassGenerator : BaseGenerator
    {
        private const string IDENT4_SPACES = "    ";
        private const string PROPERTY_VARIABLE_DEFINITION = "constexpr char {0}PropertyClass::{1}[];";
        private const string PROPERTY_TEMPLATE = "    static constexpr char {0}[] = \"{1}\";";
        private const string PROPERTY_REGISTRATION_TEMPLATE = "        prop = PropertyObjectClassPtr::Create{0}PropInfo({1}, {2}, {3});";
        private const string PROPERTY_FUNC_REGISTRATION_TEMPLATE = "        prop = PropertyObjectClassPtr::Create{0}PropInfo({1}, {2}, true, false, {3});";
        private const string PROPERTY_TYPE_REGISTRATION_TEMPLATE = "        prop = PropertyObjectClassPtr::CreatePropInfo({0}, {1}, {2});";
        private const string DEFAULT_PROPERTY_OBJECT_BASE = "GenericPropertyObjectImpl";
        private const string DEFAULT_PROPERTY_OBJECT_TEMPLATE = "template<typename... TInterfaces>";
        private const string BASE_PROPERTY_OBJECT_INTERFACE = "IPropertyObject";
        private const string PROPERTY_OBJECT_BASE_TEMPLATE = "{0}PropertyObject";
        private const string INCLUDE_FORMAT_QUOTED = "#include \"{0}.h\"";
        private const string INCLUDE_FORMAT = "#include {0}";
        private const string INCLUDE_FORMAT_LIBRARY = "#include <{0}/{1}.h>";

        private readonly IRTInterface _rtClass;
        private readonly StringBuilder _headers;
        private readonly bool _inheritsBasePropertyObject;

        public PropertyClassGenerator(IRTInterface rtClass, IGeneratorOptions options) : base((IGeneratorOptions) options.Clone())
        {
            Options.GenerateWrapper = false;
            _rtClass = rtClass;

            _headers = new StringBuilder();
            _inheritsBasePropertyObject = _rtClass.Type.PropertyClass.PrivateImplementation ||
                                          (_rtClass.BaseType.Name == BASE_PROPERTY_OBJECT_INTERFACE);
        }

        public override IVersionInfo Version => new VersionInfo
        {
            Major = 5,
            Minor = 0,
            Patch = 0
        };

        private string GetPropertyRegistration(IProperty property)
        {
            string propType = "Object";
            string propValues = property.Values ?? "nullptr";

            StringBuilder registration = new StringBuilder();

            switch (property.Type)
            {
                case CoreType.Int:
                    propType = property.IsEnum.GetValueOrDefault()
                                   ? "Enum"
                                   : property.Type.ToString();
                    break;
                case CoreType.Bool:
                case CoreType.Float:
                case CoreType.String:
                    propType = property.Type.ToString();
                    break;
                case CoreType.List:
                    registration.AppendLine(string.Format(PROPERTY_TYPE_REGISTRATION_TEMPLATE, property.VariableName, "ctList", property.DefaultValue));
                    break;
                case CoreType.Dictionary:
                    registration.AppendLine(string.Format(PROPERTY_TYPE_REGISTRATION_TEMPLATE, property.VariableName, "ctDict", property.DefaultValue));
                    break;
                case CoreType.Ratio:
                    registration.AppendLine(string.Format(PROPERTY_TYPE_REGISTRATION_TEMPLATE, property.VariableName, "ctRatio", property.DefaultValue));
                    break;
                case CoreType.Function:
                case CoreType.Procedure:
                {
                    propType = property.Type == CoreType.Function ? "Func" : "Proc";
                    registration.AppendLine(string.Format(PROPERTY_FUNC_REGISTRATION_TEMPLATE, propType, property.VariableName, property.DefaultValue, property.Visible.GetValueOrDefault() ? "true" : "false"));
                    break;
                }
                case CoreType.Object:
                    // Third arg in createObjectProp() is "enabled" not "values"
                    if (property.Enabled.HasValue)
                    {
                        propValues = property.Enabled.Value.ToString();
                    }
                    break;
                case CoreType.BinaryData:
                    break;
                case CoreType.Undefined:
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }

            if (registration.Length == 0)
            {
                registration.AppendLine(string.Format(PROPERTY_REGISTRATION_TEMPLATE, propType, property.VariableName, property.DefaultValue, propValues));
            }

            if (!property.Visible.GetValueOrDefault())
            {
                registration.AppendLine("        prop.setVisible(false);");
            }

            if (property.ReadOnly.GetValueOrDefault())
            {
                registration.AppendLine("        prop.setReadOnly(true);");

                if (property.ReadOnlyValue != null)
                {
                    registration.AppendLine($"        prop.setReadOnlyValue({property.ReadOnlyValue});");
                }
            }

            if (property.StaticConfigurationAction.HasValue)
            {
                registration.AppendLine($"        prop.setConfigurationAction(ConfigurationMode::Static, PropertyState::Exists | PropertyState::Missing | PropertyState::New, ConfigurationAction::{property.StaticConfigurationAction.Value});");
            }

            registration.AppendLine("        propClass.addPropertyInfo(prop);");

            return registration.ToString();
        }

        private string GetPropertyVariable(IProperty prop, string variable)
        {
            ITypeName typeName = prop.Method.Arguments[0].Type;

            switch (variable)
            {
                case "SmartPtr":
                    return typeName.Wrapper.NameFull;
                case "Interface":
                    return typeName.FullName();
                case "MethodName":
                    return prop.Method.Name;
                case "OutParamType":
                    IArgument arg = prop.Method.GetLastByRefArgument();
                    return arg.Type.Flags.IsValueType
                        ? arg.Type.FullName()
                        : arg.Type.FullName(false) + "*";
                case "FirstInParamCast":
                {
                    if (prop.Method.Arguments.Count > 1 && !typeName.Flags.IsValueType)
                    {
                        return $".asPtr<{typeName.FullName()}>(true)";
                    }

                    return string.Empty;
                }
                default:
                    return null;
            }
        }

        private string GetFunctionPropertyWrapper(IProperty property)
        {
            if (property.Type != CoreType.Procedure)
            {
                return null;
            }

            IMethod method = property.Method;
            string className = $"{_rtClass.Type.PropertyClass.ClassName}PropertyClassImpl";

            switch (method.Arguments.Count)
            {
                case 0:
                    return $"Procedure([this] (IBaseObject*) {{ return {method.Name}(); }})";
                case 1:
                    return GenerateOneParamFunctionWrapper(property, className);
                case 2:
                    if (!method.Arguments[0].IsOutParam && method.Arguments[1].IsOutParam)
                    {
                        return GenerateGetterFunctionWrapper(property);
                    }
                    goto default;
                default:
                    //RenderTemplate(property, Utility.GetTemplate("cpp.property.func.array.wrapper.template"), GetPropertyVariable);
                    //return "nullptr";
                    return null;
            }
        }

        private string GenerateOneParamFunctionWrapper(IProperty property, string className)
        {
            IArgument methodArgument = property.Method.Arguments[0];
            ITypeName argType = methodArgument.Type;

            string argTypeName = argType.Name;
            if (argTypeName == "IBaseObject" || argTypeName == "void")
            {
                return $"Procedure(std::bind(&{className}:{property.Method.Name}, this, _1))";
            }

            string template;
            if (methodArgument.IsOutParam)
            {
                template = Utility.GetTemplate("cpp.property.func.wrapper.value.out.template");
            }
            else
            {
                template = Utility.GetTemplate(argType.Flags.IsValueType
                    ? "cpp.property.func.wrapper.value.template"
                    : "cpp.property.func.wrapper.template"
                );
            }

            return RenderFileTemplate(
                       property,
                       template,
                       GetPropertyVariable
                   ) + IDENT4_SPACES;
        }

        private string GenerateGetterFunctionWrapper(IProperty property)
        {
            return RenderFileTemplate(
                       property,
                       Utility.GetTemplate("cpp.property.func.getter.wrapper.template"),
                       GetPropertyVariable
                   ) + IDENT4_SPACES;
        }

        private void SetPropertyVariables(string headerFile)
        {
            IPropertyClass propertyClass = _rtClass.Type.PropertyClass;

            if (propertyClass.ConstructorArguments != null)
            {
                SetCtorArgs(propertyClass.ConstructorArguments);
            }

            if (string.IsNullOrEmpty(propertyClass.ImplementationBase))
            {
                GetBasePropertyObjectImpl(propertyClass);
            }
            else
            {
                _headers.AppendLine(!string.IsNullOrEmpty(Options.LibraryInfo?.Name)
                                        ? string.Format(INCLUDE_FORMAT_LIBRARY, Options.LibraryInfo.Name.ToLowerInvariant(), propertyClass.ImplementationBase.ToLowerSnakeCase())
                                        : string.Format(INCLUDE_FORMAT_QUOTED, propertyClass.ImplementationBase.ToLowerSnakeCase()));

                if (propertyClass.ConstructorArguments == null)
                {
                    if (propertyClass.IsImplementationTemplated)
                    {
                        Variables.Add("PropClassCtorArgs", "const StringPtr& className");
                        Variables.Add("PropClassCtorArgNames", "className");
                    }
                    else
                    {
                        Variables.Add("PropClassCtorArgs", "");
                        Variables.Add("PropClassCtorArgNames", $"{propertyClass.ClassName}PropertyClass::ClassName");
                    }
                }
            }

            if (!Variables.ContainsKey("PropClassCtorArgs"))
            {
                Variables.Add("PropClassCtorArgs", "");
            }

            if (!Variables.ContainsKey("PropClassCtorArgNames"))
            {
                Variables.Add("PropClassCtorArgNames", "");
            }

            AddPropertyObjectInterface();

            Variables.Add("PropertyClassParent", (propertyClass.ParentClassName != "PropertyObject") && !propertyClass.PrivateImplementation
                                                     ? propertyClass.ParentClassName
                                                     : "");

            Variables.Add("PropertyClassName", propertyClass.ClassName);
            Variables.Add("PropertyObjectImplBase", propertyClass.ImplementationBase);
            Variables.Add("PropertyClassHeaderFile", Path.GetFileName(headerFile));
            GetHeaders();

            if (propertyClass.IsImplementationTemplated)
            {
                Variables.Add("PropertyClassImplTemplate",
                    string.IsNullOrEmpty(propertyClass.ImplementationTemplate)
                        ? DEFAULT_PROPERTY_OBJECT_TEMPLATE
                        : propertyClass.ImplementationTemplate
                );

                Variables.Add("InterfaceImpl", "");
            }
            else
            {
                Variables.Add("InterfaceImpl", _rtClass.Type.FullName() + ", ");
            }

            ParseProperties(propertyClass);
        }

        private void SetCtorArgs(IList<IArgument> ctorArgs)
        {
            string args = string.Join(", ", ctorArgs.Select(arg => $"{(arg.IsConst ? "const " : "")}{arg.Type} {arg.Name}"));
            Variables.Add("PropClassCtorArgs", args);

            string argNames = string.Join(", ", ctorArgs.Select(arg => arg.Name));
            Variables.Add("PropClassCtorArgNames", argNames);
        }

        private void ParseProperties(IPropertyClass propertyClass)
        {
            StringBuilder propertyNames = new StringBuilder();
            StringBuilder propertyRegistrations = new StringBuilder();
            StringBuilder propertyVariableDefinitions = new StringBuilder();
            StringBuilder propertyClassVariables = new StringBuilder();
            StringBuilder propertyClassFieldInit = new StringBuilder();
            StringBuilder propertyFunctionsInit = new StringBuilder();

            foreach (IProperty property in _rtClass.Methods
                                                   .Where(m => m.Property != null &&
                                                               !m.Property.Skip &&
                                                               !m.Property.IsPropertySetter.GetValueOrDefault())
                                                   .Select(m => m.Property)
                                                   .Concat(propertyClass.AdditionalProperties))
            {
                propertyNames.AppendLine(string.Format(PROPERTY_TEMPLATE, property.VariableName, property.Name));
                propertyRegistrations.AppendLine(GetPropertyRegistration(property));
                propertyVariableDefinitions.AppendLine(
                    string.Format(
                        PROPERTY_VARIABLE_DEFINITION,
                        propertyClass.ClassName,
                        property.VariableName
                    )
                );

                if (property.NoField)
                {
                    continue;
                }

                string implField = property.GetImplementationField();
                if (!string.IsNullOrEmpty(implField))
                {
                    propertyClassVariables.AppendLine(implField);
                }

                string fieldInit = property.GetFieldInitialization();
                if (!string.IsNullOrEmpty(fieldInit))
                {
                    propertyClassFieldInit.AppendLine("        , " + fieldInit);
                }
                else if (property.Type == CoreType.Procedure)
                {
                    string function = GetFunctionPropertyWrapper(property);

                    if (!string.IsNullOrEmpty(function))
                    {
                        propertyFunctionsInit.AppendLine(
                            $"        this->setPropertyValue(StringPtr({property.VariableName}), {function.TrimTrailingNewLine()});"
                        );
                        propertyFunctionsInit.AppendLine();
                    }
                }
            }

            propertyNames.TrimTrailingNewLines();
            propertyRegistrations.TrimTrailingNewLines();
            propertyVariableDefinitions.TrimTrailingNewLines();
            propertyClassFieldInit.TrimTrailingNewLines();
            propertyFunctionsInit.TrimTrailingNewLines();

            Variables.Add("PropertyClassPropertyNames", propertyNames.ToString());
            Variables.Add("PropertyRegistrations", propertyRegistrations.ToString());
            Variables.Add("PropertyVariableDefinitions", propertyVariableDefinitions.ToString());
            Variables.Add("PropertyClassVariables", propertyClassVariables.ToString());
            Variables.Add("PropertyClassVariableInit", propertyClassFieldInit.ToString());
            Variables.Add("FuncPropertySetters", propertyFunctionsInit.ToString());
        }

        private void GetHeaders()
        {
            _headers.AppendLine(string.Format(INCLUDE_FORMAT_LIBRARY, _rtClass.Type.LibraryName, _rtClass.Type.Wrapper.IncludeName));

            if (!_inheritsBasePropertyObject)
            {
                _headers.AppendLine(string.Format(INCLUDE_FORMAT_LIBRARY, _rtClass.BaseType.LibraryName, _rtClass.BaseType.DefaultIncludeName + "_property_class"));
            }
            else
            {
                _headers.AppendLine(string.Format(INCLUDE_FORMAT, @"<coreobjects/property_object.h>"));
                _headers.AppendLine(string.Format(INCLUDE_FORMAT, @"<coreobjects/property_object_impl.h>"));
            }

            foreach (string additionalInclude in RtFile.AttributeInfo.AdditionalHeaders)
            {
                _headers.AppendLine(string.Format(INCLUDE_FORMAT, additionalInclude));
            }

            Variables.Add("headers", _headers.ToString());
        }

        private void GetBasePropertyObjectImpl(IPropertyClass propertyClass)
        {
            propertyClass.ImplementationBase = !_inheritsBasePropertyObject
                ? string.Format(PROPERTY_OBJECT_BASE_TEMPLATE, _rtClass.BaseType.NonInterfaceName)
                : DEFAULT_PROPERTY_OBJECT_BASE;

            if (propertyClass.IsImplementationTemplated)
            {
                GetBasePropertyObjectTemplated(propertyClass);
            }
            else
            {
                if (_rtClass.BaseType.Name != BASE_PROPERTY_OBJECT_INTERFACE && !propertyClass.PrivateImplementation)
                {
                    propertyClass.ImplementationBase = $"{_rtClass.BaseType.NonInterfaceName}PropertyObjectImplBase";

                    Variables.Add("PropertyObjectImplBaseTemplateArgs", $"<{_rtClass.Type.Name}>");
                    Variables.Add("PropImpl", "");
                }
                else
                {
                    if (propertyClass.ConstructorArguments != null)
                    {
                        Variables.Add("PropClassCtorArgs", "");
                    }

                    Variables.Add("PropImpl", $"using PropImpl = {propertyClass.ImplementationBase}<TInterfaces...>;");
                }

                if (propertyClass.ConstructorArguments != null || propertyClass.PrivateImplementation)
                {
                    Variables.Add("PropClassCtorArgNames", $"{propertyClass.ClassName}PropertyClass::ClassName");
                }
            }
        }

        private void GetBasePropertyObjectTemplated(IPropertyClass propertyClass)
        {
            Variables.Add("PropertyObjectImplBaseTemplateArgs", "<TInterfaces...>");

            if (_inheritsBasePropertyObject)
            {
                if (propertyClass.ConstructorArguments == null)
                {
                    Variables.Add("PropClassCtorArgs", "const StringPtr& className");
                    Variables.Add("PropClassCtorArgNames", "className");
                }

                Variables.Add("PropImpl", $"using PropImpl = {propertyClass.ImplementationBase}<TInterfaces...>;");
            }
            else
            {
                if (propertyClass.ConstructorArguments == null)
                {
                    Variables.Add("PropClassCtorArgs", "");
                    Variables.Add("PropClassCtorArgNames", $"{propertyClass.ClassName}PropertyClass::ClassName");
                }

                Variables.Add("PropImpl", "");
            }
        }

        private void AddPropertyObjectInterface()
        {
            if (_inheritsBasePropertyObject)
            {
                Variables.Add("PropertyObjectInterface", BASE_PROPERTY_OBJECT_INTERFACE + ", ");
            }
            else
            {
                Variables.Add("PropertyObjectInterface", "");
            }
        }

        protected override string GetMethodVariable(IMethod method, string variable)
        {
            if (variable == "Override")
            {
                return "override";
            }

            return CppGenerator.GetCppMethodVariable(method, variable);
        }

        protected override string GetMethodArgumentVariable(IArgument arg, IOverload overload, string variable)
        {
            if (!arg.Type.Flags.IsValueType && variable == "ArgTypeFull")
            {
                string argType = arg.Type.FullName(false);
                if (!arg.Type.Flags.IsValueType)
                {
                    argType += "*";
                }

                return argType;
            }

            return CppGenerator.GetCppMethodArgumentVariable(arg, variable, Options);
        }

        protected override bool HandleMemberMethod(IRTInterface rtClass, IMethod method)
        {
            return method.Property == null || !method.Property.Skip;
        }

        public void Generate()
        {
            if (string.IsNullOrEmpty(Options.Filename))
            {
                string fileName = Options.Filename
                                  ?? Path.GetFileNameWithoutExtension(RtFile.SourceFileName)
                                  ?? RtFile.SourceFileName;

                if (!string.IsNullOrEmpty(Options.GeneratedExtension) && Options.GeneratedExtension[0] != '.')
                {
                    Options.GeneratedExtension = '.' + Options.GeneratedExtension;
                }

                Options.Filename = fileName;
            }

            string path = Path.Combine(Options.OutputDir, Options.Filename);
            string headerFile = path + "_property_class.h";

            SetVariables(_rtClass, Utility.GetTemplate("cpp.template"));
            SetPropertyVariables(headerFile);

            string templatePath = Utility.GetTemplate("cpp.property.class.template");

            StreamReader template = null;
            try
            {
                FileInfo info = new FileInfo(templatePath);
                template = new StreamReader(info.Open(FileMode.Open, FileAccess.Read, FileShare.Read));

                using (StreamWriter output = new StreamWriter(headerFile))
                {
                    while (!template.EndOfStream)
                    {
                        string templateLine = template.ReadLine();

                        if (string.IsNullOrEmpty(templateLine))
                        {
                            output.WriteLine(templateLine);
                            continue;
                        }

                        string outputLine = ReplacementRegex.Replace(templateLine, m =>
                        {
                            string variable = m.Groups[1].Value;

                            if (Variables.ContainsKey(variable))
                            {
                                return Variables[variable];
                            }

                            LogIgnoredVariable(variable, templatePath);
                            return string.Empty;
                        });

                        output.WriteLine(outputLine);
                    }
                }

                template.Close();
            }
            catch
            {
                template?.Dispose();
                throw;
            }
        }

        public override void GenerateFile(string templatePath)
        {
            throw new NotImplementedException();
        }

        public override string Generate(string templatePath)
        {
            throw new NotImplementedException();
        }

        public override void RegisterTypeMappings(Dictionary<string, string> mappings)
        {
        }

        protected override StringBuilder WriteMethodWrappers(IRTInterface rtClass, string templatePath)
        {
            throw new NotImplementedException();
        }
    }
}
