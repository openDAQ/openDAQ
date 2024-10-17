using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;

using RTGen.Generation;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;
using RTGen.Types;
using RTGen.Types.Doc;
using RTGen.Util;


// Ignore Spelling: retval coretypes Dict nullptr cpp paramref stylesheet ok Ixxxx ies Ansi endregion Params


namespace RTGen.CSharp.Generators
{
    class CSharpGenerator : TemplateGenerator
    {
        private const string SAMPLE_READER    = "ISampleReader";
        private const string MULTI_READER     = "IMultiReader";
        private const string BLOCK_READER     = "IBlockReader";
        private const string START_INDEX      = "startIndex";
        private const string START_INDEX_DECL = " nuint " + START_INDEX;

        private IGeneratorOptions _options;
        private string            _licenseFilePath;
        private bool              _useArgumentPointers; //to decorate argument variables with cast and native pointer getter
        private bool              _isFactory;
        private bool              _isBasedOnSampleReader;
        private bool              _isMultiReader;
        private bool              _isBlockReader;
        private ITypeName         _currentClassType;
        private string            _currentBaseClassName;
        private string            _currentMethodName;
        private string            _lastOutParamForDocComment;
        private List<string>      _alreadyProcessedProperties = new List<string>();
        private bool              _hasResultProperty;

        private readonly Dictionary<string, string>                     _fileNameMappings            = new Dictionary<string, string>();
        private readonly Dictionary<string, string>                     _specialReMappings           = new Dictionary<string, string>();

        private readonly Dictionary<string, string>                     _dotNetClassInterfaces       = new Dictionary<string, string>();
        private readonly Dictionary<string, string>                     _defaultArgumentValues       = new Dictionary<string, string>();
        private readonly Dictionary<string, string>                     _genericTypeParameters       = new Dictionary<string, string>();
        private readonly Dictionary<string, string>                     _castOperatorTypes           = new Dictionary<string, string>();
        private readonly Dictionary<string, string>                     _manualFactories             = new Dictionary<string, string>();
        private readonly Dictionary<string, string>                     _enumTypes                   = new Dictionary<string, string>();
        private readonly List<string>                                   _keyWords                    = new List<string>();
        private readonly Dictionary<string, Dictionary<string, string>> _renamedParameters           = new Dictionary<string, Dictionary<string, string>>();
        private readonly Dictionary<string, string>                     _callingConventions          = new Dictionary<string, string>();
        private readonly Dictionary<string, string>                     _delegateTypeReplacements    = new Dictionary<string, string>();
        private readonly List<string>                                   _factoryEnumTypesToIgnore    = new List<string>();
        private readonly Dictionary<string, string>                     _factoryArgumentDefaults     = new Dictionary<string, string>();

        private readonly List<string>                                   _defaultMethodsToHideWithNew = new List<string>();

        private readonly Dictionary<IMethod, string> _rawMethodNames = new Dictionary<IMethod, string>();

        //get <text> encapsulated in `<text>` or '<text>' ignoring "'s" or "`s"
        private readonly Regex  _replaceDocApostrophWithCodeTags            = new Regex(@"(?(['`]s\s)@@|(['`])(?<text>.+?)\1)");
        private readonly string _replaceDocApostrophWithCodeTagsReplacement = "<c>${text}</c>";
        private readonly Regex  _getDocHtmlTags                             = new Regex(@"</?[abceip](?(\s)\s.+?>|>)");
        private readonly Regex  _getDocReference                            = new Regex(@"(?<type>\w+?)\s""(?<display>.+?)""(?<rest>.*)$");
        private readonly Regex  _extractWordPlusRest                        = new Regex(@"(\W??)(?<word>\w+)\1(?<rest>\s|\W.*)$");
        private readonly Regex  _argumentsSeparatorRegEx                    = new Regex(@",(?=(((?!\>).)*\<)|[^\<\>]*$)");   //find all commas NOT in template arguments (<x,y>)
        private readonly Regex  _argumentSplitRegEx                         = new Regex(@"\s+(?=(((?!\>).)*\<)|[^\<\>]*$)"); //find all whitespace NOT in template arguments (<x,y>)

        private readonly VersionInfo _version = new VersionInfo()
        {
            Major = 1,
            Minor = 0,
            Patch = 0
        };

        /// <summary>Generator version info.</summary>
        /// <returns>Returns the generator version info.</returns>
        public override IVersionInfo Version => _version;

        /// <summary>Additional generation options (file name, suffix, output dir ...).</summary>
        public override IGeneratorOptions Options
        {
            get => _options;
            set
            {
                _options = value;

                //adapt library name
                _options.LibraryInfo.Name = _options.LibraryInfo.Name.Capitalize();

                //adapt extension
                if (string.IsNullOrEmpty(_options.GeneratedExtension))
                {
                    _options.GeneratedExtension = ".cs";
                }

                base.IndentationSpaces = 4;
            }
        }

        /// <summary>Object description of the source file.</summary>
        public override IRTFile RtFile
        {
            get => base.RtFile;
            set
            {
                base.RtFile = value;

                IRTInterface currentClass = value.CurrentClass;
                _options.Filename = currentClass.Type.NonInterfaceName;

                //check if there exists a mapping for the (type) name
                // (temporarily get the CSharp mappings here since they have not been registered yet)
                Dictionary<string, string> typeMappings = new Dictionary<string, string>();
                RegisterTypeMappings(typeMappings);
                if (_fileNameMappings.TryGetValue(_options.Filename.ToLowerInvariant(), out string mapping)) //handle exceptional names
                {
                    _options.Filename = mapping;
                }
                else
                {
                    string typeMapping = typeMappings.FirstOrDefault(kvp => kvp.Key.Equals(_options.Filename, StringComparison.OrdinalIgnoreCase)).Value;
                    if (!string.IsNullOrEmpty(typeMapping))
                    {
                        _options.Filename = typeMapping;
                    }
                }

                IAttributeInfo info = value.AttributeInfo;
                info.NamespaceSeparator = ".";
                info.ParameterSeparator = ",";

                _currentClassType      = currentClass.Type;
                _currentBaseClassName  = currentClass.BaseType.UnmappedName;
                _isBasedOnSampleReader = (_currentBaseClassName == SAMPLE_READER);
                _isMultiReader         = _isBasedOnSampleReader && (_currentClassType.UnmappedName == MULTI_READER);
                _isBlockReader         = _isBasedOnSampleReader && (_currentClassType.UnmappedName == BLOCK_READER);
            }
        }

        private IOptions<string> RtFileTypeMappings => this.RtFile?.AttributeInfo.TypeMappings ?? new Options<string>();


        public override void RegisterTypeMappings(Dictionary<string, string> mappings)
        {
            /*  //from openDAQ\core\coretypes\include\coretypes\common.h
                typedef uint32_t    ErrCode;
                typedef uint8_t     Bool;
                typedef int64_t     Int;
                typedef double      Float;
                typedef char*       CharPtr;
                typedef const char* ConstCharPtr;
                typedef void*       VoidPtr;
                typedef size_t      SizeT;
                typedef uint32_t    EnumType;
            */

            //keys case sensitive
            mappings.Add("ErrCode",       "ErrorCode");
            mappings.Add("Bool",          "bool");
            mappings.Add("Int",           "long");
            mappings.Add("UInt",          "ulong");
            mappings.Add("uint64_t",      "ulong");
            mappings.Add("Float",         "double");
            mappings.Add("CharPtr",       "string");
            mappings.Add("ConstCharPtr",  "string");
          //mappings.Add("VoidPtr",       "void*"); //not working, needs special handling
            mappings.Add("SizeT",         "nuint"); //C# >= V9.0
            mappings.Add("EnumType",      "int");

            mappings.Add("IntfID",        "Guid");

            mappings.Add("IList",         "IListObject");
            mappings.Add("List",          "ListObject");
            mappings.Add("IDict",         "IDictObject");
            mappings.Add("Dict",          "DictObject");
            mappings.Add("IBoolean",      "IBoolObject");
            mappings.Add("Boolean",       "BoolObject");
            mappings.Add("IInteger",      "IIntegerObject");
            mappings.Add("Integer",       "IntegerObject");
            mappings.Add("INumber",       "INumberObject");
            mappings.Add("Number",        "NumberObject");
            mappings.Add("IFloat",        "IFloatObject");
          //mappings.Add("Float",         "FloatObject"); //see above, needs special handling
            mappings.Add("IString",       "IStringObject");
            mappings.Add("String",        "StringObject");
            mappings.Add("IVersion",      "IOpenDaqVersion");
            mappings.Add("Version",       "OpenDaqVersion");
            mappings.Add("IEvent",        "IDaqEvent");
            mappings.Add("Event",         "DaqEvent");
            mappings.Add("IEventArgs",    "IDaqEventArgs");
            mappings.Add("EventArgs",     "DaqEventArgs");
            mappings.Add("IEventHandler", "IDaqEventHandler");
            mappings.Add("EventHandler",  "DaqEventHandler");
            mappings.Add("IType",         "IDaqType");
            mappings.Add("Type",          "DaqType");
            mappings.Add("ICoreType",     "ICoreTypeObject");
            mappings.Add("CoreType",      "CoreTypeObject");

            //custom mappings
            //don't need to be set twice
            if (_fileNameMappings.Count == 0)
            {
                //keys are case sensitive

                //keys must be lowercase here
                _fileNameMappings.Add("boolean",          "BoolObject");
                _fileNameMappings.Add("float",            "FloatObject");

                _specialReMappings.Add("double",          "FloatObject"); //'Float' is class AND typedef thus mapped to 'double'

                _dotNetClassInterfaces.Add("IDictObject", "IDictObject");
                _dotNetClassInterfaces.Add("IIterator",   "IEnumerator");
                _dotNetClassInterfaces.Add("IListObject", "IListObject");

                _defaultArgumentValues.Add("nullptr",     "null");

                //format ("<typeName>", "<TName>:<constraintType>[,<TName>:<constraintType>]") without '[]' which marks optional continuation
                _genericTypeParameters.Add("IDictObject",          "TKey:BaseObject,TValue:BaseObject"); //no blank spaces because of Split()
              //_genericTypeParameters.Add("IEvent",               "TValue:BaseObject,T:BaseObject");
                _genericTypeParameters.Add("IIterable",            "TValue:BaseObject");
                _genericTypeParameters.Add("IIterator",            "TValue:BaseObject");
                _genericTypeParameters.Add("IListObject",          "TValue:BaseObject");
              //_genericTypeParameters.Add("IProperty",            "TValue:BaseObject");
              //_genericTypeParameters.Add("IPropertyObject",      "TValue:BaseObject");
                _genericTypeParameters.Add(SAMPLE_READER + "Base", "TValue:struct,TDomain:struct");

                _castOperatorTypes.Add("Boolean", "bool");
                _castOperatorTypes.Add("Float",   "double");
                _castOperatorTypes.Add("Integer", "long");
                _castOperatorTypes.Add("String",  "string");

                _manualFactories.Add("Number", "CreateNumberObject");

                _enumTypes.Add("CoreType", "CoreType");

                _delegateTypeReplacements.Add("FuncCall", "FuncCallDelegate");
                _delegateTypeReplacements.Add("ProcCall", "ProcCallDelegate");

                //ignore certain factory argument types
                _factoryEnumTypesToIgnore.Add("SampleType"); //replaced using `OpenDAQFactory.GetSampleType<TElementType>()`

                //default values for certain factory argument types (or names)
                _factoryArgumentDefaults.Add("ReadMode",                "ReadMode.Scaled");
                _factoryArgumentDefaults.Add("ReadTimeoutType",         "ReadTimeoutType.All");
                _factoryArgumentDefaults.Add("startOnFullUnitOfDomain", "false");
                _factoryArgumentDefaults.Add("requiredCommonSampleRate", "-1");
                _factoryArgumentDefaults.Add("minReadCount", "1");

                _keyWords.Add("base");
                _keyWords.Add("event");
                _keyWords.Add("params");
                _keyWords.Add("string");

                string[] callingConventionNames = Enum.GetNames(typeof(CallingConvention));
                foreach (string callingConventionName in callingConventionNames)
                {
                    _callingConventions.Add(callingConventionName.ToLower(), callingConventionName);
                }

                _defaultMethodsToHideWithNew.Add("Equals");
                //_defaultMethodsToHideWithNew.Add("GetType"); //special handling in 'GetMethodVariable("Name")'
                _defaultMethodsToHideWithNew.Add("GetHashCode");
                _defaultMethodsToHideWithNew.Add("ToString");
            }
        }

        /// <summary>Called after every-time the variables are reset, before the start of the generation process.</summary>
        protected override void SetCustomVariables()
        {
            //base.Variables["Namespace"] = string.Join(".", (_options.LibraryInfo.Namespace?.Components ?? System.Array.Empty<string>()).Take(3).Select(str => str.Capitalize())); //PascalCase for the first 3 namespace components
            base.Variables["Namespace"] = "Daq.Core." + _options.LibraryInfo.Name.Replace("Core", string.Empty);

            FixVariableMapping("BaseTypeNonInterface");
            FixVariableMapping("NonInterfaceType");

            //set new variable CC = camelCase
            base.Variables["NonInterfaceTypeCC"] = base.Variables["NonInterfaceType"].Uncapitalize();

            base.Variables["CSUsings"] = GetUsings(base.Variables["Interface"]);


            //=== local functions =================================================================

            void FixVariableMapping(string variable)
            {
                if (this.RtFileTypeMappings.TryGet(base.Variables[variable], out string mapping))
                {
                    //apply mapping
                    base.Variables[variable] = mapping;

                    //special (e.g. double for FloatObject)
                    if (_specialReMappings.TryGetValue(mapping, out mapping))
                    {
                        base.Variables[variable] = mapping;
                    }
                }
            }

            string GetUsings(string interfaceName)
            {
                StringBuilder usings = new StringBuilder();

                if (_options.LibraryInfo.Name != "CoreTypes")
                {
                    usings.AppendLine();
                    usings.AppendLine();
                    usings.AppendLine("using Daq.Core.Types;");

                    if (_options.LibraryInfo.Name != "CoreObjects")
                    {
                        usings.AppendLine("using Daq.Core.Objects;");
                    }

                    //never needed: usings.AppendLine("using Daq.Core.OpenDaq;");
                }

                return usings.ToString();
            }
        }

        /// <summary>Called when all variables have been set before main output generation.</summary>
        /// <remarks>You can look-up predefined variables in <see cref="BaseGenerator.SetVariables"/> or associated methods.</remarks>
        protected override void OnVariablesReady()
        {
        }

        /// <summary>Get the value of the specified variable in the currently generated interface.</summary>
        /// <param name="rtClass">The RT interface that is being generated.</param>
        /// <param name="variable">The variable to replace.</param>
        /// <returns>Returns <c>null</c> if the is unknown or unhandled otherwise the replaced variable or empty string.</returns>
        /// <remarks>If the returned value is NOT <c>null</c> it effectively overrides the base variables.<br/>
        /// If the returned value is <c>null</c> it checks for a base variable if exists otherwise reports a warning.</remarks>
        protected override string GetFileVariable(IRTInterface rtClass, string variable)
        {
            switch (variable)
            {
                case "LibraryName":
                    return _options.LibraryInfo.Name;

                case "CSEnumerations":
                    return GenerateEnumerations(this.RtFile);

                case "CSRawMethods":
                    return GenerateRawDelegateMethods(rtClass);

                case "CSImplementationProperties":
                    return GenerateImplementationProperties(rtClass);

                case "CSImplementationMethods":
                    return GenerateImplementationMethods(rtClass);

                case "CSExportedFactories":
                    return GenerateFactories(rtClass);

                case "CSInterfaceDoc":
                    return GetDocComment(rtClass);

                case "CSClassGenericParams":
                    return GetClassGenericParamsString(rtClass);

                case "CSClassGenericParamConstraints":
                    return GetClassGenericParamConstraints(rtClass);

                case "CSClassDotNetInterfaces":
                    return GetClassInterfaces(rtClass);

                case "CSInterfaceImplementation":
                    return GetInterfaceImplementation(rtClass);

                case "CSOperators":
                    return GenerateOperators(rtClass);

                case "CSLicenseComment":
                    return GetLicenseHeader();
            }

            return base.GetFileVariable(rtClass, variable);


            //=== local functions =================================================================

            string GetLicenseHeader()
            {
                if (string.IsNullOrEmpty(_licenseFilePath))
                {
                    string licenseFile = FindFile(_options.OutputDir, "LICENSE.txt");

                    if (!File.Exists(licenseFile))
                        licenseFile = FindFile(_options.OutputDir, "license.in");

                    if (!File.Exists(licenseFile))
                        licenseFile = FindFile(_options.OutputDir, @"shared\tools\license-header-checker\license.in");

                    if (!File.Exists(licenseFile))
                        return string.Empty;

                    _licenseFilePath = licenseFile;
                }

                return File.ReadAllText(_licenseFilePath);
            }

            string FindFile(string searchPath, string fileName)
            {
                if (string.IsNullOrEmpty(searchPath))
                    return fileName;

                string filePath = Path.Combine(searchPath, fileName);
                if (!File.Exists(filePath))
                    return FindFile(Path.GetDirectoryName(searchPath), fileName);

                return filePath;
            }
        }

        /// <summary>Get the value of the specified variable for the currently generated method.</summary>
        /// <param name="method">The method that is being generated.</param>
        /// <param name="variable">The variable to replace.</param>
        /// <returns>Returns <c>null</c> if the is unknown or unhandled otherwise the replaced variable or empty string.</returns>
        /// <remarks>If the returned value is not null it effectively overrides the base variables.<br/>
        /// If the returned value is <c>null</c> it checks for a base variable if exists otherwise reports a warning.</remarks>
        protected override string GetMethodVariable(IMethod method, string variable)
        {
            bool      isReaderFunction = IsReaderFunction(method);
            bool      isProperty       = IsProperty(method);
            IArgument lastOutParam     = isProperty ? GetPropertyAsArgument(method) : method.GetLastByRefArgument();
            string[]  voidArgNames     = isReaderFunction ? GetVoidArgumentNames(method).ToArray() : System.Array.Empty<string>();

            switch (variable)
            {
                case "LibraryName":
                    return _options.LibraryInfo.Name;

                case "Name":
                    return GetDotNetMethodName();

                case "CSRawMethodName":
                    return _rawMethodNames.TryGetValue(method, out string methodName) ? methodName : method.Name.Capitalize();

                case "CSRawName":
                    return method.Name;

                case "CSPropertyName":
                    return GetPropertyName(method);

                case "CSImplementationPropertyGetter":
                    return GenerateImplementationPropertyGetter(method);

                case "CSImplementationPropertySetter":
                    return GenerateImplementationPropertySetter(method);

                case "ReturnType":
                    return GetDotNetTypeName(method.ReturnType) ?? string.Empty;

                case "CSReturnVariable":
                    return GetDotNetTypeName(method.ReturnType, dontGetGenericParameters: true, dontCast: _isFactory).Uncapitalize() ?? string.Empty;

                case "ReturnTypePtr":
                    {
                        bool dontCastTypeName = _isFactory
                                                && !IsDotNetInterface(lastOutParam?.Type.Name)
                                                || IsEnumType(lastOutParam?.Type);
                        return GetDotNetTypeName(lastOutParam?.Type, method, lastOutParam, dontCast: dontCastTypeName) ?? string.Empty;
                    }

                case "CSRawCallingConvention":
                    return GetRawCallingConvention();

                case "CSRawReturnTypePtrDeclaration":
                    return GetRawReturnTypePtrDeclaration(method, lastOutParam);

                case "CSCastArgumentObjects":
                    return GetCastArgumentObjects(method.Overloads[0], lastOutParam);

                case "CSInitReturnArg":
                    return GetArgumentName(method.Overloads[0], lastOutParam) + " = default;";

                case "CSReturnArgName":
                    return GetArgumentName(method.Overloads[0], lastOutParam) ?? string.Empty;

                case "CSNonInterfaceReturnTypePtr":
                    return GetDotNetTypeName(lastOutParam.Type, method, lastOutParam, dontCast: true);

                case "CSReturnPointerValidation":
                    return GetReturnPointerValidation();

                case "CSTemporaryReturnValueObject":
                    return GetTemporaryReturnValueObject();

                case "CSReturnValue":
                    return GetReturnValue();

                case "CSNativeOutputArgument":
                    return DoNativeOutputArgument();

                case "CSReturnTheOutputValue":
                    return DoReturnTheOutputValue();

                case "CSReturnOrVoidCall":
                    return (lastOutParam != null) ? "return " : string.Empty;

                case "CSDefaultReturnValue":
                    return DoDefaultReturnValue();

                case "CSArgumentsWithoutLastOut":
                    return GetArgumentsWithoutLastOut();

                case "Arguments":
                    return RenderCSharpArguments(method);

                case "CSArgumentsReader":
                    return GetReaderArguments(GetMethodVariable(method, "Arguments"));

                case "ArgumentNames":
                    return GetArgumentNames();

                case "CSArgumentNamesReader":
                    return GetReaderArgumentNames(GetArgumentNames(isUnmanagedCall: false));

                case "CSHideDefault":
                    return DoHideDefaultMethod() ? " new" : string.Empty;

                case "CSHideDefaultComment":
                    return DoHideDefaultMethod() ? " //'new' -> hides base member (inherited from Object)" : string.Empty;

                case "CSMethodDocReader":
                    return GetDocCommentReader();

                case "CSArrayLength":
                    return GetArrayLengthCode();

                case "CSCheckArrayTypeAndThrow":
                    return GetCheckArrayTypeAndThrowCode();

                case "CSDeclareJaggedArrayPointers":
                    return GetDeclareJaggedArrayPointersCode();

                case "CSPinJaggedArrays":
                    return GetPinJaggedArraysCode();

                case "CSFreeJaggedArrays":
                    return GetFreeJaggedArraysCode();

                case "CSFixedArrays":
                    return GetFixedArrayCode();

                case "CSCastToType":
                    return GenerateOperatorCastToType(method);

                case "CSCastType":
                    return GetCastTypeName(lastOutParam.Type);

                case "CSValueGetter":
                    return GetValueGetterName(method);

                case "CSEqualsOptions":
                    return GetEqualsOptions(lastOutParam.Type);

                case "CSCreatorFactory":
                    return GetCreatorName(this.RtFile.Factories.FirstOrDefault());

                case "CSStealRefHandling":
                    return HandleStealRefAttributes(method.Overloads[0], method.Arguments);
            }

            return base.GetMethodVariable(method, variable);


            //=== local functions =================================================================

            string GetDotNetMethodName()
            {
                string name = method.Name.Capitalize();
                if (name.Equals("GetType"))
                {
                    string lastOutParamTypeName = lastOutParam.Type.UnmappedName;
                    if (IsInterface(lastOutParamTypeName))
                        lastOutParamTypeName = lastOutParamTypeName.Substring(1);

                    if (lastOutParamTypeName.Equals("Type"))
                        lastOutParamTypeName = "DaqType";

                    name = name.Replace("Type", lastOutParamTypeName);
                }
                return name;
            }

            string GetRawCallingConvention()
            {
                //e.g. [Stdcall] for System.Runtime.CompilerServices.CallConvStdcall
                return string.IsNullOrEmpty(method.CallingConvention)
                    ? string.Empty
                    : $"[{method.CallingConvention.Capitalize()}]";
            }

            string GetReturnPointerValidation()
            {
                //not to be handled as object type?
                if (lastOutParam.Type.Flags.IsValueType
                    /*|| !lastOutParam.Type.Name.Equals("string")*/)
                //if (lastOutParam.Type.Name != "IStringObject")
                {
                    return string.Empty;
                }

                string indentation = base.Indentation + base.Indentation;

                if (isProperty)
                {
                    indentation += base.Indentation;
                }

                bool oldUseArgumentPointers = _useArgumentPointers;
                _useArgumentPointers = true;
                string returnArgName = GetArgumentName(method.Overloads[0], lastOutParam) ?? string.Empty;
                _useArgumentPointers = oldUseArgumentPointers;

                StringBuilder code = new StringBuilder();

                code.AppendLine();
                code.AppendLine(indentation + "// validate pointer");
                code.AppendLine(indentation + $"if ({returnArgName} == IntPtr.Zero)");
                code.AppendLine(indentation + "{");
                code.AppendLine(indentation + base.Indentation + "return default;");
                code.AppendLine(indentation + "}");

                //code.TrimTrailingNewLine();
                return code.ToString();
            }

            string GetTemporaryReturnValueObject()
            {
                if (_isFactory
                    || lastOutParam.Type.Flags.IsValueType
                    || !IsCastOperatorType(lastOutParam.Type.NonInterfaceName))
                {
                    return string.Empty;
                }

                string indentation        = base.Indentation + base.Indentation;
                string returnValueName    = GetMethodVariable(method, "CSReturnValue");
                string returnValueObject  = this.GetReturnValue(method, lastOutParam, dontCast: false);

                if (isProperty)
                {
                    indentation += base.Indentation;
                }

                StringBuilder code = new StringBuilder();

                code.AppendLine();
                code.AppendLine(indentation + $"using var {returnValueName} = {returnValueObject};");

                code.TrimTrailingNewLine();
                return code.ToString();
            }

            string GetReturnValue()
            {
                if (!_isFactory && !lastOutParam.Type.Flags.IsValueType
                    && IsCastOperatorType(lastOutParam.Type.NonInterfaceName))
                {
                    return GetArgumentName(method.Overloads[0], lastOutParam);
                }

                if (!lastOutParam.Type.Flags.IsValueType
                    || lastOutParam.Type.Name.Equals("string")) //handled as object type
                {
                    return this.GetReturnValue(method, lastOutParam);
                }

                return GetMethodVariable(method, "CSReturnArgName");
            }

            string GetArgumentsWithoutLastOut()
            {
                //set flag to use the last "ByRef argument" (argument.IsOutParam) as return value
                bool backup = Options.GenerateWrapper;
                Options.GenerateWrapper = true;

                string arguments = RenderCSharpArguments(method);

                Options.GenerateWrapper = backup;

                return arguments;
            }

            string GetArgumentNames(bool isUnmanagedCall = true)
            {
                _useArgumentPointers = true;
                string argumentNames = base.GetArgumentNames(method.Overloads[0], ", ", "out ");
                _useArgumentPointers = false;

                if (_isFactory)
                {
                    string methodName = method.Name;
                    List<string> names = argumentNames.Split(',').ToList();

                    for (int i = 0; i < names.Count; ++i)
                    {
                        names[i] = GetRenamedParameterName(methodName, names[i].Trim());
                    }

                    argumentNames = string.Join(", ", names).TrimStart();
                }

                if (!isUnmanagedCall || _isFactory)
                    return argumentNames;

                if (isProperty
                    && IsSetter(method)
                    && !IsCastOperatorType(lastOutParam.Type.NonInterfaceName)
                    && !IsDotNetInterface(lastOutParam.Type.Name))
                {
                    argumentNames = "value";

                    //[2024-09-04 commented out] - BaseObject has an implicit operator IntPtr(BaseObject baseObject)
                    //if (!lastOutParam.Type.Flags.IsValueType)
                    //    argumentNames += ".NativePointer";
                }

                if (isReaderFunction)
                {
                    //for SampleReaders: decorate the void pointer argument names with "ArrayPtr" and cast to IntPtr

                    List<string> names = argumentNames.Split(',').ToList();

                    int voidArgumentCount = GetVoidArgumentCount(method);

                    //the first n arguments are the void pointers
                    for (int i = 0; i < voidArgumentCount; ++i)
                    {
                        names[i] = $" (IntPtr){names[i].Trim()}ArrayPtr";
                    }

                    argumentNames = string.Join(",", names).TrimStart();
                }

                if (!string.IsNullOrWhiteSpace(argumentNames))
                    argumentNames = string.Concat(", ", argumentNames);

                //only here we need to use the NativePointer property
                return string.Concat("base.NativePointer", argumentNames);
            }

            string DoNativeOutputArgument()
            {
                if (lastOutParam == null)
                {
                    return string.Empty;
                }

                string indentation                 = base.Indentation + base.Indentation;
                string rawReturnTypePtrDeclaration = GetMethodVariable(method, "CSRawReturnTypePtrDeclaration");
                string castArgumentObjects         = GetMethodVariable(method, "CSCastArgumentObjects");

                StringBuilder codeLines = new StringBuilder();

                codeLines.AppendLine();
                codeLines.AppendLine(indentation + "//native output argument");
                codeLines.AppendLine(indentation + rawReturnTypePtrDeclaration + castArgumentObjects);

                codeLines.TrimTrailingNewLine();
                return codeLines.ToString();
            }

            string DoReturnTheOutputValue()
            {
                if (lastOutParam == null)
                {
                    return string.Empty;
                }

                string indentation                = base.Indentation + base.Indentation;
                string returnPointerValidation    = GetMethodVariable(method, "CSReturnPointerValidation");
                string temporaryReturnValueObject = GetMethodVariable(method, "CSTemporaryReturnValueObject");
                string returnValue                = GetMethodVariable(method, "CSReturnValue");

                StringBuilder codeLines = new StringBuilder();

                codeLines.AppendLine();
                codeLines.AppendLine(returnPointerValidation + temporaryReturnValueObject); //might be empty
                codeLines.AppendLine(indentation + $"return {returnValue};");

                codeLines.TrimTrailingNewLine();
                return codeLines.ToString();
            }

            string DoDefaultReturnValue()
            {
                if (lastOutParam == null)
                {
                    return string.Empty;
                }

                if (!_defaultArgumentValues.TryGetValue(lastOutParam.DefaultValue, out string defaultValue))
                {
                    defaultValue = lastOutParam.DefaultValue;
                }

                return $" {defaultValue}";
            }

            bool DoHideDefaultMethod()
            {
                return _defaultMethodsToHideWithNew.Contains(GetDotNetMethodName());
            }

            string GetReaderArguments(string arguments)
            {
                if (!isReaderFunction || _isMultiReader)
                {
                    return arguments;
                }

                //for SampleReaders: insert startIndex declaration after the void pointer arguments

                List<string> args = arguments.Split(',').ToList();

                int voidArgumentCount = GetVoidArgumentCount(method);

                args.Insert(voidArgumentCount, START_INDEX_DECL);

                arguments = string.Join(",", args);

                return arguments;
            }

            string GetReaderArgumentNames(string argumentNames)
            {
                if (!isReaderFunction || _isMultiReader)
                {
                    return argumentNames;
                }

                //for SampleReaders: insert startIndex after the void pointer argument names

                List<string> args = argumentNames.Split(',').ToList();

                int voidArgumentCount = GetVoidArgumentCount(method);

                args.Insert(voidArgumentCount, " " + START_INDEX);

                if (lastOutParam != null)
                {
                    //remove the (last) out argument
                    args.RemoveAt(args.Count - 1);
                }

                argumentNames = string.Join(",", args);

                return argumentNames;
            }

            string GetDocCommentReader()
            {
                string docComment = GetDocComment(method.Name, method.Documentation).TrimStart(); //remove first indentation as variable is already indented in template

                if (!isReaderFunction || _isMultiReader)
                {
                    return docComment;
                }

                List<string> docCommentLines = docComment
                                               .Split(new string[] { Environment.NewLine }, StringSplitOptions.None)
                                               .ToList();

                int voidArgumentCount = GetVoidArgumentCount(method);
                int paramNumber       = 0; //count occurrences
                int lastVoidParamLine = docCommentLines.IndexOf(line => line.Contains("</param>")
                                                                        && (++paramNumber == voidArgumentCount));

                docCommentLines.Insert(lastVoidParamLine + 1, base.Indentation + "/// <param name=\"" + START_INDEX + "\">The array index to start copying the samples to (<c>0</c> to <c>samples.Length-1</c>).</param>");

                return string.Join(Environment.NewLine, docCommentLines);
            }

            string GetArrayLengthCode()
            {
                if (!isReaderFunction)
                {
                    return string.Empty;
                }

                Func<string, string> getLengthCode;
                if (!_isMultiReader)
                {
                    string lengthProperty = string.Empty;
                    if (_isBlockReader)
                        lengthProperty += " / this.BlockSize";

                    getLengthCode = name => $"(nuint){name}.Length" + lengthProperty;
                }
                else
                {
                    getLengthCode = name => $"(nuint)CoreTypesHelper.GetSmallestArrayLength({name})";
                }

                string[] voidArgCodeLines = voidArgNames.Select(name => getLengthCode(name)).ToArray();

                string code = string.Join(", ", voidArgCodeLines);

                if (voidArgCodeLines.Length == 2)
                {
                    code = $"Math.Min({code})";
                }

                return code;
            }

            string GetCheckArrayTypeAndThrowCode()
            {
                if (!isReaderFunction)
                {
                    return string.Empty;
                }

                /*
                 *  string errorMsg;
                 *  if (!OpenDAQFactory.CheckSampleType<TValue>(base.ValueReadType, out errorMsg))
                 *  {
                 *      throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_INVALIDTYPE,
                 *                                 $"The samples-array type does not match the reader setting ({errorMsg}).");
                 *  }
                 */

                string[] genericParams = default;
                string[] _ = default;

                //get from stored list
                GetGenericParametersAndConstraints(_currentBaseClassName + "Base",
                                                   ref genericParams, ref _);
                string indentation = base.Indentation + base.Indentation;

                string[] checkCodeLines = new string[] //array of format strings so escape '{' and '}' when needed
                                          {
                                              "if (!OpenDAQFactory.CheckSampleType<{0}>(base.{1}ReadType, out errorMsg))",
                                              "{{",
                                              "    throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_INVALIDTYPE,",
                                              "                               $\"The {2}-array type does not match the reader setting ({{errorMsg}}).\");",
                                              "}}"
                                          };

                List<string> codeLines = new List<string>();

                codeLines.Add(indentation + "string errorMsg;");

                for (int index = 0; index < voidArgNames.Length; ++index)
                {
                    string genericParamName = genericParams[index];
                    string genericName      = genericParamName.Substring(1);
                    string voidArgName      = voidArgNames[index];

                    foreach (string codeLine in checkCodeLines)
                    {
                        codeLines.Add(indentation + string.Format(codeLine, genericParamName, genericName, voidArgName));
                    }

                    codeLines.Add(string.Empty);
                }

                return string.Join(Environment.NewLine, codeLines).TrimStart(); //remove first indentation as variable is already indented in template
            }

            string GetDeclareJaggedArrayPointersCode()
            {
                if (!_isMultiReader)
                {
                    return string.Empty;
                }

                /*
                 * GCHandle[] pinnedSampleArrayHandles  = null;
                 * IntPtr[]   pinnedSampleArrayPointers = null;
                 */
                return GetCodeUsingVoidArgNames(base.Indentation + base.Indentation,
                                                arg => $"GCHandle[] pinned{arg.Capitalize()}ArrayHandles  = null;",
                                                arg => $"IntPtr[]   pinned{arg.Capitalize()}ArrayPointers = null;");
            }

            string GetPinJaggedArraysCode()
            {
                if (!_isMultiReader)
                {
                    return string.Empty;
                }

                /*
                 * CoreTypesHelper.PinJaggedArray(samples, out pinnedSampleArrayHandles, out pinnedSampleArrayPointers);
                 */
                return GetCodeUsingVoidArgNames(base.Indentation + base.Indentation + base.Indentation,
                                                arg => $"CoreTypesHelper.PinJaggedArray({arg}, out pinned{arg.Capitalize()}ArrayHandles, out pinned{arg.Capitalize()}ArrayPointers);");
            }

            string GetFreeJaggedArraysCode()
            {
                if (!_isMultiReader)
                {
                    return string.Empty;
                }

                /*
                 * CoreTypesHelper.FreeJaggedArray(ref pinnedSampleArrayHandles, ref pinnedSampleArrayPointers);
                 */
                return GetCodeUsingVoidArgNames(base.Indentation + base.Indentation + base.Indentation,
                                                arg => $"CoreTypesHelper.FreeJaggedArray(ref pinned{arg.Capitalize()}ArrayHandles, ref pinned{arg.Capitalize()}ArrayPointers);");
            }

            string GetFixedArrayCode()
            {
                if (!isReaderFunction)
                {
                    return string.Empty;
                }

                string[] genericParams = default;
                string[] _ = default;

                string indentation = base.Indentation + base.Indentation + base.Indentation;

                if (_isMultiReader)
                    indentation += base.Indentation;

                //fixed statement - pin a variable for pointer operations
                //https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/statements/fixed

                if (!_isMultiReader)
                {
                    /*
                     * fixed (TValue* samplesArrayPtr = &samples[0, 0])
                     */

                    //get from stored list
                    GetGenericParametersAndConstraints(_currentBaseClassName + "Base",
                                                       ref genericParams, ref _);

                    List<string> codeLines = new List<string>();

                    for (int index = 0; index < voidArgNames.Length; ++index)
                    {
                        string startIndex = START_INDEX;
                        if (_isBlockReader)
                            startIndex += " * this.BlockSize";

                        string code = $"fixed ({genericParams[index]}* {voidArgNames[index]}ArrayPtr = &{voidArgNames[index]}[{startIndex}])";
                        codeLines.Add(indentation + code);
                    }

                    return string.Join(Environment.NewLine, codeLines).TrimStart(); //remove first indentation as variable is already indented in template
                }
                else
                {
                    /*
                     * fixed (IntPtr* samplesArrayPtr = &jaggedSampleArrayPointers[0])
                     */

                    return GetCodeUsingVoidArgNames(indentation,
                                                    arg => $"fixed (IntPtr* {arg}ArrayPtr = &pinned{arg.Capitalize()}ArrayPointers[0])");
                }
            }

            string GetCastTypeName(ITypeName lastOutParamType)
            {
                bool   isValueType          = lastOutParamType.Flags.IsValueType;
                string lastOutParamTypeName = isValueType ? lastOutParamType.Name : lastOutParamType.NonInterfaceName;

                if (!isValueType && _castOperatorTypes.ContainsKey(lastOutParamTypeName))
                {
                    //cast source type -> get target type name
                    lastOutParamTypeName = _castOperatorTypes[lastOutParamTypeName];
                }
                else if (!_castOperatorTypes.ContainsValue(lastOutParamTypeName))
                {
                    //not a cast target type name
                    return $"#{lastOutParamType.NonInterfaceName}#"; //not found, return something syntactical illegal
                }

                return lastOutParamTypeName; //e.g. "double"
            }

            string GetValueGetterName(IMethod getter)
            {
                //it is either a .NET property or a getter function

                if (IsGetter(getter))
                    return GetPropertyName(getter);

                return GetMethodVariable(getter, "Name") + "()";
            }

            string GetEqualsOptions(ITypeName lastOutParamType)
            {
                switch (lastOutParamType.Name)
                {
                    case "IStringObject":
                    case "string":
                        return ", StringComparison.Ordinal";
                }

                return string.Empty;
            }

            string GetCreatorName(IRTFactory factory)
            {
                if (factory == null)
                {
                    if (_manualFactories.TryGetValue(_currentClassType.NonInterfaceName, out string creatorName))
                        return creatorName;

                    //return $"Create{base.Variables["NonInterfaceType"]}";
                    return "#no factory found#";
                }
                else
                {
                    IMethod factoryMethod = factory.ToOverload().Method; //ToDo: check if has only one argument and if argument type fits
                    return GetMethodVariable(factoryMethod, "Name");
                }
            }

            string HandleStealRefAttributes(IOverload overload, IList<IArgument> arguments)
            {
                //check all arguments for "stealRef" attribute and add SetNativePointerToZero() calls for those arguments

                IList<IArgument> stealRefArguments = arguments.Where(arg => arg.IsStealRef).ToList();

                if (stealRefArguments.Count == 0)
                {
                    return string.Empty;
                }

                string indentation = base.Indentation + base.Indentation + base.Indentation;

                StringBuilder code = new StringBuilder();

                code.AppendLine();
                code.AppendLine($"{indentation}//invalidate the objects for the arguments with 'stealRef' attribute (protect from disposing)");

                foreach (var stealRefArgument in stealRefArguments)
                {
                    string argumentName = GetArgumentName(overload, stealRefArgument);
                    argumentName = GetRenamedParameterName(GetMethodVariable(overload.Method, "Name"), argumentName);

                    code.AppendLine($"{indentation}{argumentName}.SetNativePointerToZero();");
                }

                return code.ToString();
            }

            string GetCodeUsingVoidArgNames(string indentation, params Func<string, string>[] getCodeFunctions)
            {
                List<string> codeLines = new List<string>();

                foreach (var voidArgName in voidArgNames)
                {
                    foreach (var getCode in getCodeFunctions)
                        codeLines.Add(indentation + getCode(voidArgName));
                }

                return string.Join(Environment.NewLine, codeLines).TrimStart(); //remove first indentation as variable is already indented in template
            }
        }

        /// <summary>Get the value of the specified variable for the currently generated method argument.</summary>
        /// <param name="arg">The method argument that is being generated.</param>
        /// <param name="overload">The method overload that is being generated.</param>
        /// <param name="variable">The variable to replace.</param>
        /// <returns>Returns <c>null</c> if the is unknown or unhandled otherwise the replaced variable or empty string.</returns>
        /// <remarks>If the returned value is not null it effectively overrides the base variables.<br/>
        /// If the returned value is <c>null</c> it checks for a base variable if exists otherwise reports a warning.</remarks>
        protected override string GetMethodArgumentVariable(IArgument arg, IOverload overload, string variable)
        {
            switch (variable)
            {
                case "ArgType":
                    {
                        bool dontCastTypeName = _isFactory && arg.IsOutParam && !IsDotNetInterface(arg.Type.Name);
                        string argTypeName = GetDotNetTypeName(arg.Type, overload.Method, arg, isArgumentType: true, dontCast: dontCastTypeName);
                        if (!arg.IsOutParam && (argTypeName != "IntPtr"))
                        {
                            if (arg.Type.Flags.IsValueType && IsRefArg(arg, _useArgumentPointers))
                            {
                                argTypeName = "ref " + argTypeName;
                            }
                        }
                        return argTypeName;
                    }
            }

            return base.GetMethodArgumentVariable(arg, overload, variable);
        }

        protected override string GetArgumentName(IOverload overload, IArgument argument)
        {
            if (argument == null)
            {
                return string.Empty;
            }

            string argumentName = base.GetArgumentName(overload, argument);

            bool isValueType        = argument.Type.Flags.IsValueType;
            bool isVoidArgumentType = (argument.Type.Name == "void");
            bool isCastType         = IsCastOperatorType(argument.Type.NonInterfaceName);
            bool isDotNetInterface  = IsDotNetInterface(argument.Type.Name);
            bool isReaderFunction   = IsReaderFunction(overload.Method);

            //special handling for 'out' arguments or void or casts
            if (_useArgumentPointers && (argument.IsOutParam || isCastType || isDotNetInterface) && !isValueType
                || (isVoidArgumentType && !isReaderFunction))
            {
                //decorate out argument name that it is a pointer
                argumentName = argument.Name + "Ptr";

                if (!isCastType && !isDotNetInterface)
                {
                    AddRenamedParameter(overload.Method.Name, argument.Name, argumentName);
                }
            }

            argumentName = HandleCSharpKeyword(argumentName);

            bool hasDefaultValue = !string.IsNullOrWhiteSpace(argument.DefaultValue);

            if (_useArgumentPointers && !argument.IsOutParam) //special handling for not 'out' arguments
            {
                if (isValueType && IsRefArg(argument, useArgumentPointers: true))
                {
                    //decorate value type variable
                    argumentName = "ref " + argumentName;
                }
            }
            else if (!_useArgumentPointers && hasDefaultValue)
            {
                if (!_defaultArgumentValues.TryGetValue(argument.DefaultValue, out string defaultValue))
                {
                    defaultValue = argument.DefaultValue;
                }

                argumentName += $" = {defaultValue}";
            }

            return argumentName;
        }

        protected override StringBuilder WriteMethods(IRTInterface rtClass, string baseTemplatePath)
        {
            //set flag to use the last "ByRef argument" (argument.IsOutParam) as return value
            bool backup = Options.GenerateWrapper;
            Options.GenerateWrapper = true;

            StringBuilder methods = new StringBuilder();

            List<string> allMethods = base.WriteMethods(rtClass, baseTemplatePath)
                                      .ToString()
                                      .Split(new[] { Environment.NewLine }, StringSplitOptions.None)
                                      .ToList();
            int index = 0;
            foreach (IMethod method in rtClass.Methods)
            {
                string comment = GetDocComment(method.Name, method.Documentation);
                if (!string.IsNullOrWhiteSpace(comment))
                    methods.AppendLine(comment);
                methods.AppendLine(allMethods[index++]);
                methods.AppendLine();
            }

            methods.TrimTrailingNewLine();

            Options.GenerateWrapper = backup;

            return methods;
        }

        public override void GenerateFile(string templatePath)
        {
            //base is missing output path validation
            string outputFilePath = Path.GetDirectoryName(GetOutputFilePath());
            if (!Directory.Exists(outputFilePath))
                Directory.CreateDirectory(outputFilePath);

            base.GenerateFile(templatePath);
        }


        private string RenderFileTemplate(IMethod method, string templateFile, Func<IMethod, string, string> variableCallback)
        {
            string code = base.RenderFileTemplate(method, templateFile, variableCallback);

            if (_hasResultProperty && !_isFactory)
            {
                //fully qualify access to class Daq.Core.Types.Result as it would conflict with the `Result` property of this class
                code = code.Replace("(Result.", "(Daq.Core.Types.Result.");
            }

            return code;
        }


        private bool TryGetVariable(string key, out string value)
        {
            return base.Variables.TryGetValue(key, out value);
        }

        private string HandleCSharpKeyword(string argumentName)
        {
            if (_keyWords.Contains(argumentName))
                return "@" + argumentName;

            return argumentName;
        }

        private static bool IsInterface(string typeName)
        {
            //interfaces start with 2 uppercase letters (with first one being 'I'),
            //where non-interface type names normally don't when starting with 'I' (e.g. "InfID")
            //"IXxxx" and not "Ixxxx"? (normally an interface identifier)
            return typeName.StartsWith("I") && (typeName.Length > 1) && char.IsUpper(typeName[1]);
        }

        private static bool IsProperty(IMethod method)
        {
            //setter-only properties not handled as .NET property
            return (HasGetter(method));
            //return (method.GetSetPair != null);
        }

        private static bool HasGetter(IMethod method)
        {
            return (method.GetSetPair?.Getter != null);
        }

        private static bool HasSetter(IMethod method)
        {
            return (method.GetSetPair?.Setter != null);
        }

        private static bool IsGetter(IMethod method)
        {
            return method.Equals(method.GetSetPair?.Getter);
        }

        private static bool IsSetter(IMethod method)
        {
            return method.Equals(method.GetSetPair?.Setter);
        }

        private bool HasDelegateTypeArgument(IMethod method)
        {
            return method.Arguments.Any(arg => IsDelegateType(arg));
        }

        private bool IsDelegateType(IArgument argument)
        {
            return IsDelegateType(argument.Type.Name);
        }

        private bool IsDelegateType(string typeName)
        {
            return _delegateTypeReplacements.ContainsKey(typeName);
        }

        private string GetDelegateTypeReplacement(string typeName)
        {
            if (_delegateTypeReplacements.TryGetValue(typeName, out string replacement))
                return replacement;
            return typeName;
        }

        private static IArgument GetPropertyAsArgument(IMethod method)
        {
            return method.GetSetPair.Getter?.GetLastByRefArgument() ?? method.GetSetPair.Setter?.Arguments.First();
        }

        private string GetPropertyName(IMethod method)
        {
            return method.GetSetPair?.Name ?? string.Empty;
        }

        private bool IsCastOperatorType(string nonInterfaceName)
        {
            return _castOperatorTypes.ContainsKey(nonInterfaceName);
        }

        private bool IsDotNetInterface(string typeName)
        {
            return _dotNetClassInterfaces.ContainsKey(typeName);
        }

        private bool IsEnumType(ITypeName typeName)
        {
            if ((typeName != null) && typeName.Flags.IsValueType )
                return _enumTypes.ContainsKey(typeName.UnmappedName);

            return false;
        }

        private bool IsReaderFunction(IMethod method)
        {
            return _isBasedOnSampleReader
                   && method.Name.StartsWith("read", StringComparison.InvariantCultureIgnoreCase);
        }

        private int GetVoidArgumentCount(IMethod method)
        {
            return method.Arguments.Count(arg => arg.Type.Wrapper.Name.Equals("void*"));
        }

        private IEnumerable<IArgument> GetVoidArguments(IMethod method)
        {
            return method.Arguments.Where(arg => arg.Type.Wrapper.Name.Equals("void*"));
        }

        private IEnumerable<string> GetVoidArgumentNames(IMethod method)
        {
            return GetVoidArguments(method).Select(arg => arg.Name);
        }

        #region Documentation comments

        private static IList<IDocTag> GetDocTags(IList<IDocTag> docTagList, string csharpDocTagName)
        {
            if (docTagList == null)
                return null;

            Func<IDocTag, bool> findTag;

            switch (csharpDocTagName)
            {
                case "summary":
                    findTag = tag => (tag.TagName == "ref") && (tag.TagType == TagType.Unknown); //Hack: what if the Description also has @ref elements?
                    break;

                case "remarks":
                    findTag = tag => (tag.TagType == TagType.Description)
                                     || (tag.TagType == TagType.Unknown) && tag.RawText.StartsWith("@code");
                    break;

                case "param":
                    findTag = tag => (tag.TagType == TagType.Param);
                    break;

                case "retval":
                    findTag = tag => (tag.TagType == TagType.RetVal);
                    break;

                default:
                    return null;
            }

            return docTagList.Where(findTag).ToList();
        }

        /// <summary>
        /// Adds the renamed parameter for the given method name.
        /// </summary>
        /// <param name="methodName">Name of the method (use <see cref="GetMethodVariable(IMethod, string)"/> for internal cast arguments).</param>
        /// <param name="orgName">Original parameter name.</param>
        /// <param name="newName">The new parameter name.</param>
        private void AddRenamedParameter(string methodName, string orgName, string newName)
        {
            if (!_renamedParameters.TryGetValue(methodName, out Dictionary<string, string> renamedArguments))
            {
                renamedArguments = new Dictionary<string, string>();
                _renamedParameters.Add(methodName, renamedArguments);
            }

            if (renamedArguments.ContainsKey(orgName))
            {
                return;
            }

            renamedArguments.Add(orgName, newName);
        }

        /// <summary>
        /// Gets the name of the renamed parameter for the specified method name.
        /// </summary>
        /// <param name="methodName">Name of the method (use <see cref="GetMethodVariable(IMethod, string)"/> for internal cast arguments).</param>
        /// <param name="paramName">Name of the parameter.</param>
        /// <returns>The new parameter name (or original when not available).</returns>
        private string GetRenamedParameterName(string methodName, string paramName)
        {
            if (_renamedParameters.TryGetValue(methodName, out Dictionary<string, string> renamedParameters))
            {
                if (renamedParameters.TryGetValue(paramName, out string newName))
                {
                    return newName;
                }
            }

            return paramName;
        }

        private string GetDocComment(IRTInterface rtClass)
        {
            IDocBrief docBrief = rtClass.Documentation?.Brief;

            if (docBrief == null)
                return string.Empty;

            IList<IDocTag> docTags = rtClass.Documentation.Tags;

            _currentMethodName         = string.Empty;
            _lastOutParamForDocComment = string.Empty;

            StringBuilder csDocComment = new StringBuilder();

            ParseDocBrief(csDocComment, docBrief, docTags);
            ParseDocDescription(csDocComment, rtClass.Documentation.Description, docTags);

            return csDocComment.ToString();
        }

        private string GetDocComment(string methodName, IDocComment documentation, bool doReturnOutParam = true, bool isProperty = false)
        {
            IDocBrief docBrief = documentation?.Brief;

            if (docBrief == null)
            {
                return string.Empty;
            }

            IList<IDocTag> docTags = documentation.Tags;

            _currentMethodName         = methodName;
            _lastOutParamForDocComment = GetLastOutParam(GetDocTags(docTags, "param"));

            StringBuilder csDocComment = new StringBuilder();

            ParseDocBrief(csDocComment, docBrief, docTags, indentation: base.Indentation);
            ParseDocDescription(csDocComment, documentation.Description, docTags, indentation: base.Indentation);

            if (!isProperty)
            {
                ParseDocParams(csDocComment, docTags, methodName, doReturnOutParam, indentation: base.Indentation);
                ParseDocRetVal(csDocComment, docTags, doReturnOutParam, indentation: base.Indentation);
            }

            csDocComment.TrimTrailingNewLine();

            return csDocComment.ToString();


            //=== local functions =================================================================

            string GetLastOutParam(IList<IDocTag> paramTags)
            {
                if ((paramTags == null) || (paramTags.Count == 0))
                    return string.Empty;

                foreach (IDocParam docParam in paramTags)
                {
                    if (doReturnOutParam && docParam.IsOut)
                        return docParam.ParamName;
                }

                return string.Empty;
            }
        }

        private string GetDocComment(IEnumeration _)
        {
            //enumeration documentation is stored in the leading documentation list
            //ToDo: how to determine which documentation belongs to which enumeration?
            var leadingDocs = this.RtFile.LeadingDocumentation.FirstOrDefault(doc => doc.Brief != null);
            if (leadingDocs == null)
                return string.Empty;

            IDocBrief docBrief = leadingDocs.Brief;

            _currentMethodName         = string.Empty;
            _lastOutParamForDocComment = string.Empty;

            StringBuilder csDocComment = new StringBuilder();

            csDocComment.AppendLine();

            ParseDocBrief(csDocComment, docBrief);

            csDocComment.TrimTrailingNewLine();

            return csDocComment.ToString();
        }

        private void ParseDocBrief(StringBuilder csDocComment,
                                   IDocBrief docBrief,
                                   IList<IDocTag> docTags = null,
                                   string indentation = "")
        {
            string csTagName = "summary";

            List<string> csDocCommentLines = new List<string>();

            //summary lines from documentation.Brief
            ParseDocLines(csDocCommentLines, null, docBrief.Lines);

            if (docTags != null)
            {
                //additional summary lines from Documentation.Tags (if any)
                foreach (var docTag in GetDocTags(docTags, csTagName))
                {
                    ParseDocTag(csDocCommentLines, docTag);
                }
            }

            AppendDocCommentLines(csDocComment, csDocCommentLines,
                                  indentation, csTagName, csTagAttributes: string.Empty);
        }

        private void ParseDocDescription(StringBuilder csDocComment,
                                         IDocDescription docDescription,
                                         IList<IDocTag> docTags,
                                         string indentation = "")
        {
            string csTagName = "remarks";

            if (docDescription?.Lines == null)
                return;

            List<string> csDocCommentLines = new List<string>();

            //remarks lines from documentation.Description
            ParseDocLines(csDocCommentLines, null, docDescription.Lines);

            //additional remarks lines from Documentation.Tags (if any)
            foreach (var docTag in GetDocTags(docTags, csTagName))
            {
                ParseDocTag(csDocCommentLines, docTag);
            }

            AppendDocCommentLines(csDocComment, csDocCommentLines,
                                  indentation, csTagName, csTagAttributes: string.Empty);
        }

        private void ParseDocParams(StringBuilder csDocComment,
                                    IList<IDocTag> docTags,
                                    string methodName,
                                    bool doReturnOutParam,
                                    string indentation = "")
        {
            docTags = GetDocTags(docTags, "param");

            if ((docTags == null) || (docTags.Count == 0))
                return;

            List<string> csDocCommentLines = new List<string>();

            foreach (IDocParam docParam in docTags.Cast<IDocParam>())
            {
                csDocCommentLines.Clear();

                ParseDocLines(csDocCommentLines, string.Empty, docParam.Lines);

                if (doReturnOutParam && docParam.IsOut)
                {
                    AppendDocCommentLines(csDocComment, csDocCommentLines,
                                          indentation, "returns", csTagAttributes: string.Empty);
                }
                else
                {
                    string paramName = GetRenamedParameterName(methodName, docParam.ParamName);

                    AppendDocCommentLines(csDocComment, csDocCommentLines,
                                          indentation, "param", csTagAttributes: $" name=\"{paramName}\"");
                }
            }
        }

        private void ParseDocRetVal(StringBuilder csDocComment,
                                    IList<IDocTag> docTags,
                                    bool doReturnOutParam,
                                    string indentation = "")
        {
            docTags = GetDocTags(docTags, "retval");

            if ((docTags == null) || (docTags.Count == 0))
                return;

            List<string> csDocCommentLines = new List<string>();

            foreach (IDocRetVal docRetVal in docTags)
            {
                csDocCommentLines.Clear();

                ParseDocLines(csDocCommentLines, string.Empty, docRetVal.Lines);

                if (doReturnOutParam)
                {
                    csDocCommentLines.Insert(0, $"<c>OpenDaqException(ErrorCode.{docRetVal.ReturnValue})</c>");
                    AppendDocCommentLines(csDocComment, csDocCommentLines,
                                          indentation, "exception",
                                          csTagAttributes: $" cref=\"OpenDaqException\"");
                }
                else
                {
                    csDocCommentLines.Insert(0, $"<c>ErrorCode.{docRetVal.ReturnValue}</c>");
                    AppendDocCommentLines(csDocComment, csDocCommentLines,
                                          indentation, "returns", csTagAttributes: string.Empty);
                }
            }
        }

        private static void AppendDocCommentLines(StringBuilder csDocComment,
                                                  List<string> csDocCommentLines,
                                                  string indentation,
                                                  string docTagName,
                                                  string csTagAttributes)
        {
            //remove trailing empty line
            if (csDocCommentLines.Last().Length == 0)
                csDocCommentLines.RemoveAt(csDocCommentLines.Count - 1);

            if (csDocCommentLines.Count == 1)
            {
                //one-liner
                csDocComment.AppendLine($"{indentation}/// <{docTagName}{csTagAttributes}>{csDocCommentLines.First()}</{docTagName}>");
            }
            else
            {
                //multi-liner
                csDocComment.AppendLine($"{indentation}/// <{docTagName}{csTagAttributes}>");

                foreach (string docCommentLine in csDocCommentLines)
                    csDocComment.AppendLine($"{indentation}/// {docCommentLine}");

                csDocComment.AppendLine($"{indentation}/// </{docTagName}>");
            }
        }

        private void ParseDocLines(IList<string> csDocComment,
                                   string docTagName,
                                   IList<IDocLine> lines)
        {
            foreach (IDocLine line in lines)
            {
                ParseDocElements(csDocComment, docTagName, line.Elements);

                docTagName = string.Empty; //reset, as only the first line contains "unknown" tag information

                if ((csDocComment.Last().Length > 0))
                {
                    csDocComment.Add(string.Empty);
                }
                else if ((line.FullText.Length == 0) && (csDocComment.Count > 1))
                {
                    AppendDocText(csDocComment, "<para/>"); //add paragraph separator
                    csDocComment.Add(string.Empty);
                }
            }
        }

        private void ParseDocElements(IList<string> csDocComment,
                                      string docTagName,
                                      IList<IDocElement> elements)
        {
            foreach (IDocElement element in elements)
            {
                switch (element.ElementType)
                {
                    case ElementType.Text:
                        ParseDocText(csDocComment, docTagName, element.RawText);
                        break;

                    //case ElementType.Unknown:
                    //    break;

                    case ElementType.Tag:
                        ParseDocTag(csDocComment, (IDocTag)element);
                        break;

                    default:
                        LogWarning($"{nameof(CSharpGenerator)}.{nameof(ParseDocElements)}() - unhandled element type: {element.ElementType}");
                        System.Diagnostics.Debug.Print("+++> unhandled element type <{0}>", element.ElementType);
                        break;
                }
            }
        }

        private void ParseDocTag(IList<string> csDocComment,
                                 IDocTag tagElement)
        {
            switch (tagElement.TagType)
            {
                case TagType.Unknown:
                    if (tagElement.TagName != "code")
                    {
                        ParseDocLines(csDocComment, tagElement.TagName, ((IUnknownTag)tagElement).Lines);
                    }
                    else
                    {
                        string codeSnippet = EscapeForXml(tagElement.RawText)
                                             .Replace("@code", "<code>")
                                             .Replace("@endcode", "</code>")
                                             .Replace(" *", "///"); //" *" is probably not consistently used

                        if (!codeSnippet.Contains("/// </code>"))
                            codeSnippet = codeSnippet.Replace("</code>", "/// </code>");

                        AppendDocText(csDocComment, codeSnippet);
                        csDocComment.Add(string.Empty);
                    }
                    break;

                //case TagType.Brief:
                //    break;

                //case TagType.Throws:
                //    break;

                //case TagType.Param:
                //    break;

                case TagType.ParamRef:
                    ParseParamRefTag(csDocComment, tagElement);
                    break;

                //case TagType.RetVal:
                //    break;

                //case TagType.Private:
                //    break;

                case TagType.Description:
                    ParseDocLines(csDocComment, string.Empty, ((IDocDescription)tagElement).Lines);
                    break;

                //case TagType.Block:
                //    break;

                default:
                    LogWarning($"{nameof(CSharpGenerator)}.{nameof(ParseDocTag)}() - unhandled tag type: {tagElement.TagType} ({tagElement.TagName})");
                    System.Diagnostics.Debug.Print("+++> unhandled tag type <{0}> ({1})", tagElement.TagType, tagElement.TagName);
                    break;
            }
        }

        private void ParseDocText(IList<string> csDocComment,
                                  string docTagName,
                                  string rawText)
        {
            /*
             * @a - The following word is marked up as a function/method parameter (usually the end result that the reader of the documentation sees is the same as if @e had been used, but the stylesheet in use could change this)
             * @e - The following word is marked up as emphasized
             * @b - The following word is marked up as as bold
             * @c - The following word is marked up as code
             * @p - The following word is rendered in a non-proportional font
             * @ref - The following word is a type reference
             */

            switch (docTagName)
            {
                case "a":
                    ParseTextNextWordPlusRest(csDocComment, csTagName: "paramref", rawText);
                    break;

                case "b":
                case "c":
                    ParseTextNextWordPlusRest(csDocComment, docTagName, rawText);
                    break;

                case "e":
                    ParseTextNextWordPlusRest(csDocComment, csTagName: "i", rawText);
                    break;

                case "ref":
                    ParseDocRefText(csDocComment, rawText);
                    break;

                case "":
                case null:
                    AppendDocText(csDocComment, EscapeForXml(rawText));
                    break;

                default:
                    LogWarning($"{nameof(CSharpGenerator)}.{nameof(ParseDocText)}() - unhandled {nameof(docTagName)}: {docTagName}");
                    System.Diagnostics.Debug.Print("+++> unknown tag '{0}'", docTagName);
                    AppendDocText(csDocComment, EscapeForXml(rawText));
                    break;
            }
        }

        private void ParseTextNextWordPlusRest(IList<string> csDocComment,
                                               string csTagName,
                                               string rawText)
        {
            Match match = _extractWordPlusRest.Match(rawText);
            if (match.Success)
            {
                string word = match.Groups["word"].Value;
                string rest = EscapeForXml(match.Groups["rest"].Value);
                AppendDocText(csDocComment, $"<{csTagName}>{word}</{csTagName}>{rest}");
            }
            else
                AppendDocText(csDocComment, EscapeForXml(rawText));
        }

        private void ParseDocRefText(IList<string> csDocComment,
                                     string rawText)
        {
            string result = string.Empty;

            var matches = _getDocReference.Matches(rawText);
            if (matches.Count == 0)
            {
                AppendDocText(csDocComment, EscapeForXml(rawText));
                return;
            }

            foreach (Match match in matches)
            {
                result = GetReferenceFromMatch(match, out string rest);

                if (string.IsNullOrEmpty(rest))
                    continue;

                result += rest;
            }

            AppendDocText(csDocComment, result);


            //=== local functions =================================================================

            string GetReferenceFromMatch(Match match, out string trailingText)
            {
                string type = match.Groups["type"].Value;
                string display = EscapeForXml(match.Groups["display"].Value);

                trailingText = EscapeForXml(match.Groups["rest"].Value);

                if (this.RtFile.AttributeInfo.TypeMappings.TryGet(type, out string mappedType))
                {
                    type = mappedType;
                }

                if (IsInterface(type))
                {
                    type = type.Substring(1); //cut the 'I'
                }

                result += $"<see cref=\"{type}\">{display}</see>";

                return result;
            }
        }

        private void ParseParamRefTag(IList<string> csDocComment, IDocTag tagElement)
        {
            string paramName = ((IDocParamRef)tagElement).ParamName;
            string rest = string.Empty;

            Match match = _extractWordPlusRest.Match(paramName);
            if (match.Success)
            {
                paramName = match.Groups["word"].Value;
                rest      = EscapeForXml(match.Groups["rest"].Value);
            }

            paramName = GetRenamedParameterName(_currentMethodName, paramName);

            if (!_lastOutParamForDocComment.Equals(paramName))
                AppendDocText(csDocComment, $"<paramref name=\"{paramName}\"/>{rest}");
            else
                AppendDocText(csDocComment, $"<c>{paramName}</c> {rest}");
        }

        private string EscapeForXml(string text)
        {
            text = HandleTextBetweenMatches(_getDocHtmlTags.Matches(text));
            text = _replaceDocApostrophWithCodeTags.Replace(text, _replaceDocApostrophWithCodeTagsReplacement);
            //text = text.Replace("nullptr", "null"); //.NET replacement
            return text;


            //=== local functions =================================================================

            string DoEscape(string rawText)
            {
                return System.Security.SecurityElement.Escape(rawText);
            }

            string HandleTextBetweenMatches(MatchCollection matches)
            {
                string newText = string.Empty;

                if (matches.Count == 0)
                    newText = DoEscape(text);
                else
                {
                    int index = 0;
                    foreach (Match match in matches)
                    {
                        if (index < match.Index)
                            newText += DoEscape(text.Substring(index, match.Index - index));

                        newText += match.Value;
                        index = match.Index + match.Length;
                    }

                    if (index < text.Length)
                        newText += DoEscape(text.Substring(index));
                }

                return newText;
            }
        }

        private static void AppendDocText(IList<string> csDocComment, string text)
        {
            if (csDocComment.Count == 0)
            {
                csDocComment.Add(text);
                return;
            }

            //add separating space if same line
            if (csDocComment[csDocComment.Count - 1].Length > 0)
                text = " " + text;

            csDocComment[csDocComment.Count - 1] += text;
        }

        #endregion Documentation comments

        private string GetClassGenericParamsString(IRTInterface rtClass)
        {
            if (GetGenericParameters(rtClass.Type, out string[] genericParams, out _))
            {
                return $"<{string.Join(", ", genericParams)}>";
            }

            return string.Empty;
        }

        private string GetClassGenericParamConstraints(IRTInterface rtClass, string indentation = null)
        {
            if (GetGenericParameters(rtClass.Type, out string[] genericParams, out string[] constraintTypes))
            {
                if (indentation == null)
                    indentation = base.Indentation;

                int index = 0;

                var constraints = genericParams
                                  .Select(param => indentation + $"where {param} : {constraintTypes[index++]}" + Environment.NewLine);
                return string.Join(string.Empty, constraints);
            }

            return string.Empty;
        }

        #region Enumerations

        private string GenerateEnumerations(IRTFile rtFile)
        {
            if ((rtFile.Enums?.Count ?? 0) == 0)
                return string.Empty;

            string templatePath = Utility.GetTemplate(Options.Language + ".enumeration.template");

            StringBuilder enumerations = new StringBuilder();

            foreach (IEnumeration enumeration in rtFile.Enums)
            {
                string comment = GetDocComment(enumeration);
                if (!string.IsNullOrWhiteSpace(comment))
                    enumerations.Append(comment);
                enumerations.AppendLine(RenderFileTemplate(enumeration, templatePath, GetEnumerationVariable));
            }

            return enumerations.ToString();
        }

        private string GetEnumerationVariable(IEnumeration enumeration, string variable)
        {
            switch (variable)
            {
                case "CSEnumType":
                    return enumeration.Name;

                case "CSEnumElements":
                    return GetEnumerations(enumeration.Options);
            }

            return string.Empty;
        }

        private string GetEnumerations(IList<IEnumOption> options)
        {
            StringBuilder enumerationOptions = new StringBuilder();

            foreach (IEnumOption option in options)
            {
                if (option.HasValue)
                    enumerationOptions.AppendLine($"{base.Indentation}{option.Name} = {option.Value},");
                else
                    enumerationOptions.AppendLine($"{base.Indentation}{option.Name},");
            }

            enumerationOptions.TrimTrailingNewLine();

            return enumerationOptions.ToString();
        }

        #endregion Enumerations

        // The base template engine only provides callbacks for generating methods once
        // But in the C# example given they need to be generated 2x (Raw and C# implementation)

        #region Raw delegates

        private string RenderRawCSharpArguments(IMethod factoryMethod, bool isWithVariable = false)
        {
            IList<string> arguments = factoryMethod.Arguments.Select(argument => GetRawArgumentType(argument)).ToList();

            if (isWithVariable)
            {
                //here type + variable name are rendered, so don't set '_useArgumentPointers=true'
                for (int i = 0; i < arguments.Count; ++i)
                {
                    arguments[i] = arguments[i] + " " + GetArgumentName(factoryMethod.Overloads[0], factoryMethod.Arguments[i]);
                }
            }

            return string.Join(", ", arguments);
        }

        private string GetRawArgumentType(IArgument arg)
        {
            string typeName         = arg.Type.Name;
            string unmappedTypeName = arg.Type.UnmappedName;
            bool   isValueType      = arg.Type.Flags.IsValueType;

            if (!isValueType)
            {
                //cannot marshal C++ objects directly so use IntPtr instead
                typeName = "IntPtr";
            }
            else
            {
                switch (unmappedTypeName)
                {
                    case "ConstCharPtr":
                        //the 'string' mapping is ok for [In] but not for [In,Out]/[Out]
                        if (IsRefArg(arg, useArgumentPointers: true))
                            typeName = "IntPtr";
                        break;

                    case "IString":
                    case "VoidPtr":
                        typeName = "IntPtr";
                        break;

                    default:
                        if (IsEnumType(arg.Type))
                        {
                            typeName = unmappedTypeName;
                        }
#if DEBUGx
                        string msg = $"+++> GetRawArgumentType unhandled value type: {unmappedTypeName} -> {typeName}";
                        System.Diagnostics.Debug.Print(msg);
                        LogWarning(msg);
#endif
                        break;
                }
            }

            if (arg.IsOutParam) //arg.Type.Modifiers == "**"
            {
                return $"out {typeName}";
            }
            else if (isValueType && IsRefArg(arg, useArgumentPointers: true))
            {
                return $"ref {typeName}";
            }
#if DEBUGx
            //is this an unhandled case?
            else if (arg.Type.Modifiers == "*")
                System.Diagnostics.Debugger.Break();
#endif

            return typeName;
        }

        private string GenerateRawDelegateMethods(IRTInterface rtClass)
        {
            StringBuilder rawMethods = new StringBuilder();

            string templatePath = Utility.GetTemplate(Options.Language + ".method.raw.template");
            foreach (IMethod method in rtClass.Methods)
            {
                if (!_rawMethodNames.ContainsKey(method))
                {
                    string methodName = GetMethodVariable(method, "Name");
                    int methodNameCount = _rawMethodNames.Count(kvp => kvp.Value.Equals(methodName));
                    if (methodNameCount > 0)
                        methodName += (methodNameCount + 1).ToString();
                    _rawMethodNames.Add(method, methodName);
                }

                rawMethods.AppendLine(base.Indentation + "//" + GetRawDeclaration(method.Overloads[0]));
                rawMethods.Append(base.Indentation + RenderFileTemplate(method, templatePath, (theMethod, variable) =>
                {
                    switch (variable)
                    {
                        case "Name":
                            return _rawMethodNames[theMethod];
                        case "Arguments":
                            return GetUnmanagedArguments(RenderRawCSharpArguments(theMethod));
                    }

                    return GetMethodVariable(theMethod, variable);
                }));
            }

            //determine whether this class has a Result property (getter) as this is needed to fully qualify access to static class Daq.Core.Types.Result for failure checking
            _hasResultProperty = _rawMethodNames.Keys.Any(method => (method.GetSetPair?.Getter != null) && (method.GetSetPair?.Name == "Result"));

            rawMethods.TrimTrailingNewLine();
            return rawMethods.ToString();


            //=== local functions =================================================================

            string GetUnmanagedArguments(string arguments)
            {
                if (string.IsNullOrWhiteSpace(arguments))
                    return string.Empty;

                return ", " + arguments;
            }
        }

        private static string GetRawDeclaration(IOverload overload)
        {
            string returnType = (overload.ReturnType != null) ? overload.ReturnType.Name + " " : string.Empty;
            string arguments = string.Join(", ", GetRawArguments(overload.Arguments));
            string callingConvention = (overload.Method.CallingConvention != null) ? overload.Method.CallingConvention + ";" : string.Empty;

            return $"{returnType}{overload.Method.Name}({arguments}); {callingConvention}";


            //=== local functions =================================================================

            IEnumerable<string> GetRawArguments(IList<IArgument> args)
            {
                foreach (var argument in args)
                {
                    yield return $"{(argument.IsConst ? "const " : "")}{GetRawTypeName(argument.Type)} {argument.Name}";
                }
            }

            string GetRawTypeName(ITypeName typeName)
            {
                if (string.IsNullOrEmpty(typeName.Namespace.Raw))
                    return $"{typeName.UnmappedName}{typeName.Modifiers}";

                return $"{typeName.Namespace}.{typeName.UnmappedName}{typeName.Modifiers}";
            }
        }

        #endregion Raw delegates

        #region Implementation Arguments

        private string RenderCSharpArguments(IMethod method)
        {
            //calls GetDotNetTypeName() through GetMethodArgumentVariable()
            string args = GetMethodArguments(method.Overloads[0],
                                             "out $ArgType$ $ArgName$",
                                             "$ArgType$ $ArgName$",
                                             ", ");

            if (HasDelegateTypeArgument(method))
            {
                args = HandleDelegateArguments(SplitArguments(args.Trim(), ',').ToList());
            }

            if (_isFactory && _isBasedOnSampleReader)
            {
                args = HandleSampleReaderFactoryArguments(SplitArguments(args.Trim(), ',').ToList());
            }

            return args;


            //=== local functions =================================================================

            string HandleSampleReaderFactoryArguments(List<string> argsList)
            {
                //filter out arguments with certain enum types
                argsList = argsList.Where(arg => !_factoryEnumTypesToIgnore.Contains(arg.TrimStart().Split(' ')[0])).ToList();

                //add defaults to some arguments
                for (int index = 0; index < argsList.Count; ++index)
                {
                    string   arg            = argsList[index];
                    string[] argTypeAndName = SplitArguments(arg.Trim(), ' ');

                    if (argTypeAndName.Length > 2)
                        continue; // no out/ref arguments

                    if (!_factoryArgumentDefaults.TryGetValue(argTypeAndName[0], out string defaultValue)
                        && ((argTypeAndName.Length >= 2) && !_factoryArgumentDefaults.TryGetValue(argTypeAndName[1], out defaultValue)))
                        continue;

                    argsList[index] = string.Concat(arg, " = ", defaultValue);
                }

                return string.Join(",", argsList);
            }

            string HandleDelegateArguments(List<string> argsList)
            {
                //change type name of delegate arguments
                for (int index = 0; index < argsList.Count; ++index)
                {
                    string   arg            = argsList[index];
                    string[] argTypeAndName = SplitArguments(arg.Trim(), ' ');

                    int typeIndex = 0;
                    if (argTypeAndName.Length > 2)
                        typeIndex = 1;

                    if (!IsDelegateType(argTypeAndName[typeIndex]))
                        continue;

                    argTypeAndName[typeIndex] = GetDelegateTypeReplacement(argTypeAndName[typeIndex]);
                    argsList[index] = string.Join(" ", argTypeAndName);
                }

                return string.Join(", ", argsList);
            }

            string[] SplitArguments(string arguments,
                                    char separator)
            {
                switch (separator)
                {
                    case ',':
                        return _argumentsSeparatorRegEx.Replace(arguments, "#")
                               .Split('#');

                    case ' ':
                        return _argumentSplitRegEx.Replace(arguments, "#")
                               .Split('#');

                    default:
                        return arguments
                               .Split(separator);
                }
            }
        }

        private string GetDotNetTypeName(ITypeName typeNameObject,
                                         IMethod method                = null,
                                         IArgument argument            = null,
                                         bool isArgumentType           = false,
                                         bool dontGetGenericParameters = false,
                                         bool dontCast                 = false)
        {
            if (typeNameObject == null)
            {
                return isArgumentType ? "IntPtr" : "void";
                //workaround for "ReturnTypePtr" having no lastOutParam (method not function) -> BaseGenerator.WriteMethods()
                //there is always a method.ReturnType, even if for CSharp there is no lastOutParam (method.GetLastByRefArgument)
                //to use -p = Options.GenerateWrapper is right now no option for CSharp as it uses the wrong template in BaseGenerator.SetVariables (naming)
            }

            //special case "cast operator type"
            if (!dontCast && _castOperatorTypes.TryGetValue(typeNameObject.NonInterfaceName, out string typeName))
            {
                return typeName;
            }

            if (dontCast || !_dotNetClassInterfaces.TryGetValue(typeNameObject.Name, out typeName))
            {
                typeName = typeNameObject.Name;

                if (!typeNameObject.Flags.IsValueType)
                {
                    if (IsInterface(typeName))
                    {
                        //remove interface-marker
                        typeName = typeName.Substring(1);
                    }
                }
                else if (_enumTypes.TryGetValue(typeNameObject.NonInterfaceName, out string enumTypeName))
                {
                    typeName = enumTypeName;
                }
            }

            if (!dontGetGenericParameters)
            {
                if (typeNameObject.HasGenericArguments)
                {
                    typeName += GetGenericParameterString(typeNameObject);
                }
                else if (TryGetGenericParametersString(typeNameObject, method, argument, out string generics))
                {
                    if (generics.StartsWith("<"))
                    {
                        if (generics.Equals($"<{argument?.Type.NonInterfaceName}>"))
                            generics = $"<{GetFirstGenericParameterFromClass()}>/*{generics}*/";

                        typeName += generics; //e.g. "ListObject<TKey>"
                    }
                    else
                    {
                        typeName = generics; //e.g. "TValue"
                    }
                }
            }

            bool isVoidPtr = typeName.Equals("void") && typeNameObject.Modifiers.StartsWith("*");

            if (isVoidPtr && typeNameObject.Modifiers.Equals("*"))
            {
                //a 'void*' argument in a SampleReader 'readXXX()' function is treated as an value array of the generic argument type ('TValue[]' or 'TDomain[]')
                if (IsReaderFunction(method)
                    && GetGenericParameters(_currentClassType, out string[] genericParams, out _))
                {
                    //get the generic parameter name for the argument
                    int voidArgumentNumber = GetVoidArguments(method)
                                             .IndexOf(arg => arg.Name == argument.Name);

                    string arrayKind = !_isMultiReader ? "[]" : "[][]";
                    typeName = genericParams[voidArgumentNumber] + arrayKind;
                }
                else
                {
                    typeName = "IntPtr";
                }
            }
            else if (typeName.Equals("VoidPtr") || isVoidPtr)
            {
                typeName = "IntPtr";
            }

            return typeName;
        }

        private bool GetGenericParameters(ITypeName type,
                                          out string[] genericParams,
                                          out string[] constraintTypes)
        {
            genericParams   = default;
            constraintTypes = default;

            if (type.HasGenericArguments)
            {
                //workaround: remove empty entries to avoid getting something like 'ISomething<>'
                var genericArgs = type.GenericArguments
                                  .Where(genericArg => !string.IsNullOrEmpty(genericArg.Name))
                                  .ToList();

                if (genericArgs.Count > 0)
                {
                    genericParams = genericArgs
                                    .Select(genArgType => GetDotNetTypeName(genArgType, dontGetGenericParameters: true, dontCast: true)
                                                          .Replace("Ptr", string.Empty)) //ToDo: problem of "PropertyObjectPtr, PropertyValueEventArgsPtr" as a single 'genericArg.Name'
                                    .ToArray();
                }
            }

            if (!type.HasGenericArguments || (genericParams == default))
            {
                string typeName = type.Name;

                if (_isBasedOnSampleReader && typeName.Equals(_currentClassType.Name))
                {
                    typeName = _currentBaseClassName + "Base";
                }

                //get from stored list
                GetGenericParametersAndConstraints(typeName, ref genericParams, ref constraintTypes);
            }
            else if (genericParams[0].Equals("BaseObject"))
            {
                genericParams = GetFirstGenericParameterFromClass()
                                .Split(','); //make array again

            }

            return (genericParams != default);
        }

        private bool GetGenericParametersAndConstraints(string typeName,
                                                        ref string[] genericParams,
                                                        ref string[] constraintTypes)
        {
            if (_genericTypeParameters.TryGetValue(typeName, out string genParams))
            {
                string[] splitParams = genParams.Split(',');
                if (splitParams.All(p => p.Contains(":")))
                {
                    genericParams = splitParams.Select(p => p.Split(':')[0]).ToArray();
                    constraintTypes = splitParams.Select(p => p.Split(':')[1]).ToArray();
                }
                else
                {
                    genericParams = splitParams;
                }

                return true;
            }

            return false;
        }

        private string GetFirstGenericParameterFromClass(string defaultName = "BaseObject")
        {
            //if (GetGenericParameters(_currentClassType, out string[] classGenericParams))
            //    return classGenericParams[0];

            return defaultName;
        }

        private string GetGenericParameterString(ITypeName argType)
        {
            if (GetGenericParameters(argType, out string[] genParams, out _))
            {
                string genericTypeList = $"<{string.Join(", ", genParams)}>";

                if (_genericTypeParameters.ContainsKey(argType.Name))
                {
                    //cannot use myself as generic parameter
                    if (genericTypeList.Equals($"<{argType.NonInterfaceName}>"))
                        genericTypeList = $"<{GetFirstGenericParameterFromClass()}>/*{genericTypeList}*/";
                }
                else
                {
                    //is not a known generic type
                    genericTypeList = $"/*{genericTypeList}*/";
                }

                return genericTypeList;
            }

            return string.Empty;
        }

        private bool TryGetGenericParametersString(ITypeName typeNameObject,
                                                   IMethod method,
                                                   IArgument argument,
                                                   out string generics)
        {
            //get the generic parameters of the class
            // a) to replace a BaseObject pointer or
            // b) to append to a listed generic type

            bool isBaseObject          = typeNameObject.Name.Equals("IBaseObject");
          //bool isListObject          = typeNameObject.Name.Equals("IListObject");
          //bool isDictObject          = typeNameObject.Name.Equals("IDictObject");
            bool isListedType          = _genericTypeParameters.ContainsKey(typeNameObject.Name);
            bool isBasedOnSampleReader = _isBasedOnSampleReader
                                         && typeNameObject.Name.Equals(_currentClassType.Name);

            if (!isBaseObject && !isListedType && !isBasedOnSampleReader)
            {
                generics = null;
                return false;
            }

            string methodName   = method?.Name;
            string argumentName = argument?.Name;

            if (argumentName != null)
            {
                //change plural into singular
                if (argumentName.EndsWith("ies"))
                    argumentName = argumentName.Substring(0, argumentName.Length - 3) + "y";
                else
                    argumentName = argumentName.TrimEnd('s');
            }

            if (!GetGenericParameterFromClass(methodName, argumentName, out generics))
            {
                if (!isListedType)
                {
                    generics = null;
                    return false;
                }

                string[] genericParameters;

                //Hack: use 'BaseObject' for each listed generic parameter
                genericParameters = _genericTypeParameters[typeNameObject.Name]
                                        .Split(',')
                                        .Select(p => "BaseObject") //replace all entries
                                        .ToArray();

                generics = string.Join(", ", genericParameters);
            }

            if (isListedType || isBasedOnSampleReader)
            {
                //to append as parameter list to a listed generic type
                generics = $"<{generics}>";
            }

            return true;
        }

        private bool GetGenericParameterFromClass(string methodName,
                                                  string argumentName,
                                                  out string genericParameter)
        {
            genericParameter = null;

            if (!GetGenericParameters(_currentClassType, out string[] genericParams, out _))
            {
                return false;
            }

            if (methodName == null)
                methodName = "@"; //some syntactically illegal name

            if (argumentName == null)
                argumentName = "@"; //some syntactically illegal name

            //check if the argument or method name indicate the generic parameter to be used
            string param = genericParams
                           .FirstOrDefault(p => p.EndsWith(argumentName,
                                                           StringComparison.InvariantCultureIgnoreCase))
                           ?? genericParams
                              .FirstOrDefault(p => methodName.Contains(p.Substring(1))); //e.g. check for 'New' of 'TNew' in 'xyzNewXyz'

            genericParameter = param ?? string.Join(", ", genericParams);

            return true;
        }

        private string GetRawReturnTypePtrDeclaration(IMethod method, IArgument lastOutParam)
        {
            string argumentType = GetRawArgumentType(lastOutParam)?
                                  .Replace("out ", string.Empty)
                                  .Replace("ref ", string.Empty)
                                  ?? string.Empty; //should never happen

            _useArgumentPointers = !lastOutParam.Type.Flags.IsValueType;
            string argumentName = GetArgumentName(method.Overloads[0], lastOutParam) ?? string.Empty;
            _useArgumentPointers = false;

            return $"{argumentType} {argumentName};" + Environment.NewLine;
        }

        private string GetCastArgumentObjects(IOverload overload, IArgument lastOutParam)
        {
            StringBuilder returnTypeDeclaration = new StringBuilder();

            string indentation  = base.Indentation + base.Indentation;
            bool   addComment   = true;
            bool   isProperty   = IsProperty(overload.Method);
            bool   isSetter     = isProperty && IsSetter(overload.Method);

            if (isProperty)
            {
                indentation += base.Indentation;
            }

            //cast argument types
            foreach (var argument in overload.Arguments)
            {
                bool isCastType               = IsCastOperatorType(argument.Type.NonInterfaceName);
                bool isDotNetInterface        = IsDotNetInterface(argument.Type.Name);
                bool isFactoryIgnoredArgument = _isFactory && _isBasedOnSampleReader && _factoryEnumTypesToIgnore.Contains(argument.Type.Name);
                bool isDelegateType           = IsDelegateType(argument);

                if (!isFactoryIgnoredArgument
                    && ((argument.Name.Equals(lastOutParam?.Name) && !isSetter)
                        || argument.Type.Flags.IsValueType
                        || !(isCastType || isDotNetInterface))
                    && !isDelegateType)
                {
                    continue;
                }

                if (addComment)
                {
                    addComment = false;
                    returnTypeDeclaration.AppendLine();

                    if (!isDelegateType)
                    {
                        returnTypeDeclaration.AppendLine(indentation + "//cast .NET argument to SDK object");
                    }
                    else
                    {
                        returnTypeDeclaration.AppendLine(indentation + "//wrap SDK delegate around .NET delegate");
                    }
                }

                string castType     = GetDotNetTypeName(argument.Type, overload.Method, argument, dontCast: true);
                string argumentName = base.GetArgumentName(overload, argument);

                if (isDelegateType)
                {
                    //-> var valueFuncCall = CoreTypesFactory.CreateFuncCallWrapper(value);
                    //-> var valueProcCall = CoreTypesFactory.CreateProcCallWrapper(value);
                    string nativeDelegateTypeName = argument.Type.Name;
                    string nativeDelegateName     = "wrapped" + argumentName.Capitalize();
                    string factoryName            = GetMethodVariable(overload.Method, "LibraryName").Equals("CoreTypes") ? string.Empty : "CoreTypesFactory.";

                    AddRenamedParameter(overload.Method.Name, argumentName, nativeDelegateName);

                    if (isProperty)
                    {
                        //override the name for the property setter
                        argumentName = "value";
                    }

                    returnTypeDeclaration.AppendLine($"{indentation}var {nativeDelegateName} = {factoryName}Create{nativeDelegateTypeName}Wrapper({HandleCSharpKeyword(argumentName)});");
                }
                else if (!isFactoryIgnoredArgument)
                {
                    //-> var packetsPtr = (ListObject<Packet>)packets;
                    string castName       = argumentName + "Ptr";
                    string usingStatement = isDotNetInterface ? string.Empty : "using ";

                    AddRenamedParameter(GetMethodVariable(overload.Method, "Name"), argumentName, castName);

                    if (isProperty)
                    {
                        //override the name for the property setter
                        argumentName = "value";
                    }

                    returnTypeDeclaration.AppendLine($"{indentation}{usingStatement}var {castName} = ({castType}){HandleCSharpKeyword(argumentName)};");
                }
                else
                {
                    //-> SampleType genericSampleType = OpenDAQFactory.GetSampleType<TElementType>();
                    string[] genericParams = default;
                    string[] _ = default;

                    //get from stored list
                    GetGenericParametersAndConstraints(_currentBaseClassName + "Base", ref genericParams, ref _);
                    string genericParameterName = genericParams.FirstOrDefault(param => argumentName.ToLowerInvariant().Contains(param.Substring(1).ToLowerInvariant()));
                    returnTypeDeclaration.AppendLine($"{indentation}{castType} {argumentName} = OpenDAQFactory.Get{castType}<{genericParameterName}>();");
                }
            }

            return returnTypeDeclaration.ToString();
        }

        #endregion Implementation Arguments

        #region ImplementationProperties

        private string GenerateImplementationProperties(IRTInterface rtClass)
        {
            StringBuilder implementationProperties = new StringBuilder();

            //set flag to use the last "ByRef argument" (argument.IsOutParam) as return value
            bool backup = Options.GenerateWrapper;
            Options.GenerateWrapper = true;

            foreach (IMethod method in rtClass.Methods)
            {
                if (!IsProperty(method))
                    continue;

                string propertyName = GetPropertyName(method);

                if (_alreadyProcessedProperties.Contains(propertyName))
                    continue;

                _alreadyProcessedProperties.Add(propertyName);

                string templatePath = Utility.GetTemplate(Options.Language + ".property.impl.template");

                string comment = GetDocComment(method.Name, method.Documentation, isProperty: true);
                if (!string.IsNullOrWhiteSpace(comment))
                    implementationProperties.AppendLine(comment);
                implementationProperties.AppendLine(RenderFileTemplate(method, templatePath, GetMethodVariable));
            }

            Options.GenerateWrapper = backup;

            if (implementationProperties.Length <= 0)
            {
                return string.Empty;
            }

            implementationProperties.TrimTrailingNewLines();

            StringBuilder code = new StringBuilder();

            code.AppendLine();
            code.AppendLine();
            code.AppendLine($"{base.Indentation}#region properties");
            code.AppendLine();
            code.AppendLine(implementationProperties.ToString());
            code.AppendLine();
            code.AppendLine($"{base.Indentation}#endregion properties");

            code.TrimTrailingNewLines();
            return code.ToString();
        }

        private string GenerateImplementationPropertyGetter(IMethod method)
        {
            if (!HasGetter(method))
                return string.Empty;

            StringBuilder implementationProperties = new StringBuilder();

            implementationProperties.AppendLine();

            string templatePath = Utility.GetTemplate(Options.Language + ".property.impl.get.template");

            //string comment = GetDocComment(method.Name, method.Documentation);
            //if (!string.IsNullOrWhiteSpace(comment))
            //    implementationProperties.AppendLine(comment);
            implementationProperties.AppendLine(RenderFileTemplate(method.GetSetPair.Getter, templatePath, GetMethodVariable));

            implementationProperties.TrimTrailingNewLines();
            return implementationProperties.ToString();
        }

        private string GenerateImplementationPropertySetter(IMethod method)
        {
            if (!HasSetter(method))
                return string.Empty;

            StringBuilder implementationProperties = new StringBuilder();

            implementationProperties.AppendLine();

            string templatePath = Utility.GetTemplate(Options.Language + ".property.impl.set.template");

            //string comment = GetDocComment(method.Name, method.Documentation);
            //if (!string.IsNullOrWhiteSpace(comment))
            //    implementationProperties.AppendLine(comment);
            implementationProperties.AppendLine(RenderFileTemplate(method.GetSetPair.Setter, templatePath, GetMethodVariable));

            implementationProperties.TrimTrailingNewLines();
            return implementationProperties.ToString();
        }

        #endregion ImplementationProperties

        #region ImplementationMethods

        private string GenerateImplementationMethods(IRTInterface rtClass)
        {
            StringBuilder implementationMethods = new StringBuilder();

            implementationMethods.AppendLine();
            implementationMethods.AppendLine();

            //set flag to use the last "ByRef argument" (argument.IsOutParam) as return value
            bool backup = Options.GenerateWrapper;
            Options.GenerateWrapper = true;

            foreach (IMethod method in rtClass.Methods)
            {
                if (IsProperty(method))
                    continue;

                bool hasRetVal          = method.ReturnsByRef()
                                          || method.Arguments.Any(argument => argument.IsOutParam);
                bool isIteratorMoveNext = (method.Arguments.Count == 0)
                                          && rtClass.Type.Name.Equals("IIterator")
                                          && method.Name.Equals("moveNext", StringComparison.InvariantCultureIgnoreCase);
                bool isSampleReader     = _isBasedOnSampleReader
                                          && method.Name.StartsWith("read", StringComparison.InvariantCultureIgnoreCase)
                                          && method.Arguments.Any(arg => arg.Type.Wrapper.Name.Equals("void*"));

                string templatePath = Utility.GetTemplate(Options.Language + ".method.impl.template");

                //special cases
                if (isSampleReader)
                {
                    if (!_isMultiReader)
                        templatePath = Utility.GetTemplate(Options.Language + ".method.impl.samplereader.template");
                    else
                        templatePath = Utility.GetTemplate(Options.Language + ".method.impl.multireader.template");
                }
                else if (hasRetVal)
                {
                    templatePath = Utility.GetTemplate(Options.Language + ".method.impl.ret.template");
                }
                else if (isIteratorMoveNext)
                {
                    templatePath = Utility.GetTemplate(Options.Language + ".method.impl.boolret.template");
                }

                string comment = GetDocComment(method.Name, method.Documentation);
                if (!string.IsNullOrWhiteSpace(comment))
                    implementationMethods.AppendLine(comment);
                implementationMethods.AppendLine(RenderFileTemplate(method, templatePath, GetMethodVariable));
            }

            Options.GenerateWrapper = backup;

            implementationMethods.TrimTrailingNewLines();
            return implementationMethods.ToString();
        }

        private string GetReturnValue(IMethod method, IArgument lastOutParam, bool dontCast = true)
        {
            string nonInterfaceReturnTypePtr = GetMethodVariable(method, "CSNonInterfaceReturnTypePtr");
            _useArgumentPointers = true;
            string returnArgName = GetArgumentName(method.Overloads[0], lastOutParam) ?? string.Empty;
            _useArgumentPointers = false;

            //preset with standard object instantiation
            string returnValue = $"new {nonInterfaceReturnTypePtr}({returnArgName}, incrementReference: false)";

            if (!dontCast && IsCastOperatorType(nonInterfaceReturnTypePtr))
            {
                returnValue = $"using var {returnArgName} = {returnValue};";
            }
            else if (lastOutParam.Type.Name.Equals("string"))
            {
                //convert unmanaged 'ConstCharPtr*' / 'const char**' to managed 'string' (Ansi)
                returnValue = $"Marshal.PtrToStringAnsi({returnArgName})";
            }
            else if (lastOutParam.Type.Name.StartsWith("void", StringComparison.InvariantCultureIgnoreCase))
            {
                returnValue = $"{returnArgName}; //ToDo: void";
            }
            else if (GetGenericParameters(_currentClassType, out string[] genericParams, out _)
                     && genericParams.Contains(nonInterfaceReturnTypePtr))
            {
                //to return an instance of a generic parameter we use a BaseObject function instead of a constructor
                returnValue = $"BaseObject.CreateInstance<{nonInterfaceReturnTypePtr}>({returnArgName}, incrementReference: false)";
            }

            return returnValue;
        }

        private string GenerateOperators(IRTInterface rtClass)
        {
            string templatePath  = Utility.GetTemplate(Options.Language + ".method.casts.template");
            StringBuilder castOperators = new StringBuilder();

            //get all "XxxValue" getters
            var valueGetters = rtClass.Methods.Where(m => IsGetter(m) && m.Name.EndsWith("value", StringComparison.InvariantCultureIgnoreCase));

            if ((valueGetters.Count() == 0) && _castOperatorTypes.TryGetValue(rtClass.Type.NonInterfaceName, out string castClassType))
            {
                //no getter ending with "value" -> get the first getter that returns the cast type
                valueGetters = rtClass.Methods.Where(m => IsGetter(m) && (m.GetLastByRefArgument()?.Type.Name == castClassType)).Take(1);
            }

            //generate the cast operators -> $CSValueGetter$, $CSCastType$, $CSCreatorFactory$
            foreach (var getter in valueGetters)
            {
                string castType = GetMethodVariable(getter, "CSCastType");
                if (string.IsNullOrWhiteSpace(castType) || castType.StartsWith("#"))
                    continue;

                castOperators.Append(RenderFileTemplate(getter, templatePath, GetMethodVariable));
            }

            if (rtClass.Type.Name.Equals("IRatio"))
            {
                //get the operators for the Ratio class
                castOperators.Append(File.ReadAllText(Utility.GetTemplate($"{Options.Language}.{rtClass.Type.Name.ToLower()}.operators.template")));
            }

            if (castOperators.Length <= 0)
            {
                return string.Empty;
            }

            StringBuilder code = new StringBuilder();

            code.AppendLine();
            code.AppendLine();
            code.AppendLine($"{base.Indentation}#region operators");
            code.AppendLine(castOperators.ToString());
            code.AppendLine($"{base.Indentation}#endregion operators");

            code.TrimTrailingNewLines();
            return code.ToString();
        }

        private string GenerateOperatorCastToType(IMethod getter)
        {
            string templatePath = Utility.GetTemplate(Options.Language + ".method.casts.totype.template");

            IRTFactory factory = this.RtFile.Factories.FirstOrDefault();
            if (factory != null)
            {
                if (factory.ToOverload().Arguments.Count > 2)
                    return string.Empty;
            }

            string castOperator = RenderFileTemplate(getter, templatePath, GetMethodVariable);

            return castOperator;
        }

        private string GetClassInterfaces(IRTInterface rtClass)
        {
            if (!_dotNetClassInterfaces.TryGetValue(rtClass.Type.Name, out string interfaceName))
                return string.Empty;

            string[] genericParameters = default;
            string[] _                 = default;

            if (!GetGenericParametersAndConstraints(rtClass.Type.Name, ref genericParameters, ref _))
                return string.Empty;

            return $", {interfaceName}<{string.Join(", ", genericParameters)}>";
        }

        private string GetInterfaceImplementation(IRTInterface rtClass)
        {
            if (!IsDotNetInterface(rtClass.Type.Name))
                return string.Empty;

            string[] genericParameters = default;
            string[] _                 = default;

            if (!GetGenericParametersAndConstraints(rtClass.Type.Name, ref genericParameters, ref _))
                return string.Empty;

            string indentation   = base.Indentation;
            string indentation_2 = indentation + indentation;

            StringBuilder interfaceImplementation = new StringBuilder();

            switch (rtClass.Type.Name)
            {
                case "IIterator":
                    AddDotNetInterfaceImplementation(interfaceImplementation, "IEnumerator");
                    break;

                case "IListObject":
                    AddDotNetInterfaceImplementation(interfaceImplementation, "IList");
                    break;

                case "IDictObject":
                    AddDotNetInterfaceImplementation(interfaceImplementation, "IDictionary");
                    break;
            }

            interfaceImplementation.TrimTrailingNewLines();

            return interfaceImplementation.ToString();


            //=== local functions =================================================================

            void AddDotNetInterfaceImplementation(StringBuilder code, string interfaceName)
            {
                string templatePath = Utility.GetTemplate($"{Options.Language}.{interfaceName.ToLowerInvariant()}.impl.template");

                IMethod dummyModel = rtClass.Methods.FirstOrDefault();

                code.AppendLine();
                code.Append(RenderFileTemplate(dummyModel, templatePath, (theMethod, variable) =>
                {
                    //variable overrides
                    switch (variable)
                    {
                        case "CSGenericClassParameters":
                            return string.Join(", ", genericParameters);

                        case "CSGenericClassParameter1":
                            return genericParameters[0];

                        case "CSGenericClassParameter2":
                            return genericParameters.Last();
                    }

                    return GetMethodVariable(theMethod, variable);
                }));
            }
        }

        #endregion ImplementationMethods

        private static bool IsRefArg(IArgument arg, bool useArgumentPointers)
        {
            string modifiers = arg.Type.Modifiers;
            return (modifiers == "*") || (useArgumentPointers && (modifiers == "&"));
        }

        //private static bool IsOutArg(IArgument arg)
        //{
        //    return arg.Type.Modifiers == "**";
        //}

        #region Factories

        private string GenerateFactories(IRTInterface rtClass)
        {
            StringBuilder generatedFactories = new StringBuilder();

            string factoryTemplatePath = Utility.GetTemplate(Options.Language + ".factory.template");
            string factoryImplTemplatePath = Utility.GetTemplate(Options.Language + ".factory.impl.template");

            _isFactory = true;

            foreach (IRTFactory factory in this.RtFile.Factories)
            {
                IMethod factoryMethod = factory.ToOverload().Method;

                //we identify generics ourselves and it is impractical to use the type GUID (IntfID)
                if (factoryMethod.Arguments.Any(arg => arg.Type.Name.Equals("IntfID") || arg.Type.Name.Equals("Guid")))
                {
                    continue; //ignore factory method
                }

                if (factoryMethod.ReturnType == null)
                {
                    //somehow factory methods do not have a ReturnType, so set it to "ErrorCode" (omitted return type means "int" in C++)
                    factoryMethod.ReturnType = new TypeName(this.RtFile.AttributeInfo,
                                                            rtClass.BaseType.Namespace.ToString(this.RtFile.AttributeInfo.NamespaceSeparator),
                                                            "ErrCode");
                }

                generatedFactories.Append(GenerateFactoryImport(rtClass, factory, factoryMethod, factoryTemplatePath));
                generatedFactories.Append(GenerateFactoryImplementation(factory, factoryMethod, factoryImplTemplatePath));
            }

            _isFactory = false;

            //don't want to have any trailing line-breaks in the result
            if (generatedFactories.Length > 0)
                generatedFactories.TrimTrailingNewLines();

            return generatedFactories.ToString();
        }

        private string GenerateFactoryImport(IRTInterface rtClass, IRTFactory factory, IMethod method, string templatePath)
        {
            StringBuilder generatedCode = new StringBuilder();

            generatedCode.AppendLine();
            generatedCode.AppendLine(base.Indentation + "//" + GetRawDeclaration(method.Overloads[0]));
            generatedCode.Append(RenderFileTemplate(factory, templatePath, (theFactory, variable) =>
            {
                //variable overrides
                switch (variable)
                {
                    case "Name":
                        return theFactory.Name;

                    case "Arguments":
                        return RenderRawCSharpArguments(method, isWithVariable: true);

                    case "CSCallingConvention":
                        return GetDllImportCallingConvention(method.CallingConvention);

                    case "CSCharSet":
                        return GetDllImportCharSet(method.Arguments);
                }
                return GetFileVariable(rtClass, variable);
            }));

            if (generatedCode.Length > 0)
            {
                //want to have exact one trailing line-break
                generatedCode.TrimTrailingNewLines();
                generatedCode.AppendLine();
            }

            return generatedCode.ToString();

            //=== local functions =================================================================

            string GetDllImportCallingConvention(string callingConvention)
            {
                if (_callingConventions.TryGetValue(callingConvention, out var result))
                    return result;

                //default
                return _callingConventions[CallingConvention.StdCall.ToString().ToLower()];
            }

            string GetDllImportCharSet(IList<IArgument> arguments)
            {
                if (arguments.Any(arg => arg.Type.UnmappedName.Equals("ConstCharPtr")
                                         || arg.Type.UnmappedName.Equals("string")))
                {
                    return ", CharSet = CharSet.Ansi";
                }

                return string.Empty;
            }
        }

        private string GenerateFactoryImplementation(IRTFactory factory, IMethod method, string templatePath)
        {
            StringBuilder generatedCode = new StringBuilder();

            AddMissingOutParamToDocumentation(factory, method.Overloads[0].Method);

            generatedCode.AppendLine();
            generatedCode.Append(RenderFileTemplate(method, templatePath, (theMethod, variable) =>
            {
                //variable overrides
                switch (variable)
                {
                    case "CSFactoryDocRetOut":
                    case "CSFactoryDoc":
                        {
                            string comment = GetDocComment(method.Name,
                                                           factory.Documentation,
                                                           doReturnOutParam: variable.Equals("CSFactoryDocRetOut"));
                            if (!string.IsNullOrWhiteSpace(comment))
                                return comment + Environment.NewLine;
                            return string.Empty;
                        }

                    case "CSClassGenericParams":
                        return GetClassGenericParamsString(this.RtFile.CurrentClass);

                    case "CSClassGenericParamConstraints":
                        return GetClassGenericParamConstraints(this.RtFile.CurrentClass,
                                                               indentation: base.Indentation + base.Indentation);
                }

                return GetMethodVariable(theMethod, variable);
            }));

            if (generatedCode.Length > 0)
            {
                //want to have exact two trailing line-breaks
                generatedCode.TrimTrailingNewLines();
                generatedCode.AppendLine();
                generatedCode.AppendLine();
            }

            return generatedCode.ToString();


            //=== local functions =================================================================

            void AddMissingOutParamToDocumentation(IRTFactory classFactory, IMethod factoryMethod)
            {
                //for class factory documentation the "last by-ref argument" is not documented explicitly
                //('out' argument, which is the return value of the function in C++ really)

                var outArg = factoryMethod.GetLastByRefArgument();
                if (outArg == null)
                    return;

                var tags = factory.Documentation?.Tags;
                if (tags == null)
                    return; //no documentation available (Tags list will always be created otherwise)

                //create new DocParam tag
                string argText = $"The '{GetDotNetTypeName(outArg.Type, factoryMethod, dontGetGenericParameters: true)}' object.";
                DocLine docLine = new DocLine();
                docLine.Elements.Add(new DocText(argText));
                DocParam docParam = new DocParam(outArg.Name, isOut: true, $"@param {outArg.Name} {argText}");
                docParam.Lines.Add(docLine);

                //find index of first Param tag (0 if not found)
                int index = 0;
                var firstParamTag = tags.FirstOrDefault(tag => (tag.TagType == TagType.Param));
                if (firstParamTag != null)
                    index = tags.IndexOf(firstParamTag);

                tags.Insert(index, docParam);
            }
        }

        #endregion Factories

        private void LogWarning(string warningText,
                                [CallerMemberName] string memberName = "",
                                [CallerFilePath] string sourceFilePath = "",
                                [CallerLineNumber] int sourceLineNumber = 0)
        {
            Log.Warning(Log.Verbose
                            ? $"{warningText} [{memberName}() at {Path.GetFileName(sourceFilePath)}:{sourceLineNumber}]."
                            : $"{warningText}."
            );
        }
    }

    static class Extensions
    {
        public static int IndexOf<TSource>(this IEnumerable<TSource> source, Func<TSource, bool> predicate)
        {
            if (source == null)
            {
                throw new ArgumentNullException("source");
            }

            if (predicate == null)
            {
                throw new ArgumentNullException("predicate");
            }

            int index = 0;
            foreach (TSource item in source)
            {
                if (predicate.Invoke(item))
                    return index;

                ++index;
            }

            return -1;
        }
    }
}
