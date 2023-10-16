using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using RTGen.Exceptions;
using RTGen.Generation;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;
using RTGen.Types;
using RTGen.Util;

namespace RTGen.Cpp.Generators
{
    public class CppGenerator : TemplateGenerator
    {
        private const string ERROR_CODE_PREFIX = "OPENDAQ_ERR_";

        private const string MUI_NAMESPACE_START = "BEGIN_MUI_NAMESPACE";
        private const string MUI_NAMESPACE_END = "END_MUI_NAMESPACE";
        private const string CALLING_CONVENTION_MACRO = "INTERFACE_FUNC";

        private const string INCLUDE_FORMAT = "#include \"{0}\"";

        private readonly Dictionary<string, string> _knownExceptions;

        public CppGenerator()
        {
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
        }

        /// <summary>Object description of the source file.</summary>
        public override IRTFile RtFile
        {
            get => base.RtFile;
            set
            {
                base.RtFile = value;

                if (base.RtFile.AttributeInfo.PtrSuffix == null)
                {
                    base.RtFile.AttributeInfo.PtrSuffix = "Ptr";
                }

                base.RtFile.AttributeInfo.PtrMappings.Add("IEvent", new SmartPtr("Event", false));
            }
        }

        public override IGeneratorOptions Options
        {
            get => base.Options;
            set
            {
                base.Options = value;

                if (string.IsNullOrEmpty(base.Options.GeneratedExtension))
                {
                    base.Options.GeneratedExtension = ".h";
                }

                if (base.Options.GenerateWrapper && string.IsNullOrEmpty(base.Options.FileNameSuffix))
                {
                    base.Options.FileNameSuffix = "_ptr";
                }
            }
        }

        public override IVersionInfo Version => new VersionInfo
        {
            Major = 5,
            Minor = 0,
            Patch = 0
        };

        protected override void SetCustomVariables()
        {
            string sourceInclude;
            if (string.IsNullOrEmpty(Options.LibraryInfo.Name))
            {
                sourceInclude = Variables["SourceFileName"];
            }
            else
            {
                 sourceInclude = string.Join("/",
                                             Options.LibraryInfo.Name?.ToLowerInvariant(),
                                             Variables["SourceFileName"]);

            }

            Variables["DocComment"] = RtFile.CurrentClass.Documentation != null
                                          ? GenerateWrapperTypeDoc(RtFile.CurrentClass.Documentation)
                                          : "";


            Variables["LeadingDocComment"] = RtFile.LeadingDocumentation.Count > 0
                                                  ? GenerateGlobalDocComment(RtFile.LeadingDocumentation)
                                                  : "";

            Variables["TrailingDocComment"] = RtFile.TrailingDocumentation.Count > 0
                                                  ? GenerateGlobalDocComment(RtFile.TrailingDocumentation)
                                                  : "";

            Variables["SourceFileInclude"] = sourceInclude;
            RtFile.AttributeInfo.CustomIncludes.Add("BaseObjectPtr", "objectptr");

            //AddSpanOverloads(RtFile.CurrentClass);
        }

        private string GenerateGlobalDocComment(IList<IDocComment> documentation)
        {
            StringBuilder sb = new StringBuilder();
            foreach (IDocComment docComment in documentation)
            {
                sb.AppendLine(GenerateWrapperTypeDoc(docComment));
                sb.AppendLine();
            }

            return sb.ToString();
        }

        private void AddSpanOverloads(IRTInterface rtClass)
        {
            foreach (IMethod method in rtClass.Methods)
            {
                if (method.Arguments.Any(arg => arg.ArrayInfo != null))
                {
                    AddMethodSpanOverloads(method);
                }
            }
        }

        private void AddMethodSpanOverloads(IMethod method)
        {
            bool extentsEqual = true;
            IArgument firstArrayArg = null;

            foreach (IArgument argument in method.Arguments)
            {
                if (argument.ArrayInfo == null)
                {
                    continue;
                }

                if (firstArrayArg == null)
                {
                    firstArrayArg = argument;
                    continue;
                }

                extentsEqual = firstArrayArg.ArrayInfo.ExtentEquals(argument.ArrayInfo);
            }

            if (extentsEqual)
            {
                AddEqualExtentSpanOverload(method, firstArrayArg);
            }
        }

        private void AddEqualExtentSpanOverload(IMethod method, IArgument arg)
        {
            List<IArgument> args = new List<IArgument>(method.Arguments);
            Overload overload = new Overload(method, OverloadType.Helper, args);

            IArray array = arg.ArrayInfo;
            List<IArray> arrays = new List<IArray>();

            int parameterIndex = -1;

            for (int i = args.Count - 1; i >= 0; i--)
            {
                if (args[i].ArrayInfo != null)
                {
                    TypeName type = new TypeName(RtFile.AttributeInfo, RtFile.AttributeInfo.CoreNamespace, "Span")
                    {
                        GenericArguments = new List<ITypeName> { args[i].Type }
                    };

                    args[i] = new Argument(type, args[i]);
                    arrays.Add(args[i].ArrayInfo);
                }
                else if (array.ExtentType == ArrayExtent.Parameter && args[i].Name == array.Reference)
                {
                    parameterIndex = i;
                    args.RemoveAt(i);
                }
            }

            if (array.ExtentType == ArrayExtent.Parameter)
            {
                foreach (IArray arr in arrays)
                {
                    arr.ParameterIndex = parameterIndex;
                    arr.Reference = $"{arg.Name}.size()";
                }
            }

            method.Overloads.Add(overload);
        }

        public override void GenerateFile(string templatePath)
        {
            base.GenerateFile(templatePath);

            foreach (IRTInterface rtClass in RtFile.Classes)
            {
                if (rtClass.Type.Flags.IsDecorated)
                {
                    DecoratorGenerator decoratorGen = new DecoratorGenerator(rtClass, Options)
                    {
                        RtFile = RtFile
                    };
                    decoratorGen.Generate();
                }

                if (rtClass.Type.PropertyClass == null)
                {
                    continue;
                }

                PropertyClassGenerator propClassGen = new PropertyClassGenerator(rtClass, Options)
                {
                    RtFile = RtFile
                };
                propClassGen.Generate();
            }
        }

        protected override string GetFileVariable(IRTInterface rtClass, string variable)
        {
            switch (variable)
            {
                case "InterfaceToSmartPtr":
                {
                    if (RtFile.AttributeInfo.PtrMappings.TryGet(rtClass.Type.Name, out ISmartPtr smartPtr) && smartPtr.IsDefaultPtr)
                    {
                        const string TEMPLATE_PATH = "cpp.ptr.specialization.template";
                        return RenderFileTemplate(rtClass,
                                                  Utility.GetTemplate(TEMPLATE_PATH),
                                                  (rtc, var) => GetGlobalVariable(rtc, TEMPLATE_PATH, var))
                               ?? "";
                    }
                    return "";
                }
                case "TemplateArgs":
                    return GetTemplateArguments(rtClass, false);
                case "PtrIntferfaceArg":
                    return "InterfaceType";
                case "PrototypeTemplate":
                    return rtClass.Type.Flags.IsGeneric
                               ? $"template <{GetTemplateArguments(rtClass)}typename {GetFileVariable(rtClass, "PtrIntferfaceArg")} = {rtClass.Type.FullName()}>"
                               : string.Empty;
                case "PtrTemplate":
                    return rtClass.Type.Flags.IsGeneric
                               ? $"template <{GetTemplateArguments(rtClass)}typename {GetFileVariable(rtClass, "PtrIntferfaceArg")}>"
                               : string.Empty;
                case "PtrInterfaceTemplate":
                    return rtClass.Type.Flags.IsGeneric
                               ? $"<{GetFileVariable(rtClass, "PtrIntferfaceArg")}>"
                               : $"<{rtClass.Type.Name}>";
                case "PtrInterfaceFullTemplate":
                    if (rtClass.Type.GenericArguments?.Count > 0)
                    {
                        return "";
                    }

                    if (rtClass.Type.Flags.IsGeneric)
                    {
                        return $"<{rtClass.Type.FullName()}>";
                    }
                    return string.Empty;
                case "UiControlMembers":
                    if (rtClass.Type.ControlTagName != null)
                    {
                        return GenerateUiControlMembers(rtClass);
                    }

                    return string.Empty;
                case "CopyEventWrappers":
                    return rtClass.Events.Count > 0 
                               ? Event.CopyWrappers(rtClass) 
                               : string.Empty; 
                case "MoveEventWrappers":
                    return rtClass.Events.Count > 0
                               ? Event.MoveWrappers(rtClass)
                               : string.Empty;
                case "EventWrappers":
                    return rtClass.Events.Count > 0
                               ? Event.Wrappers(rtClass)
                               : string.Empty;
                case "EventWrappersCopyAssign":
                    return rtClass.Events.Count > 0
                               ? Event.WrappersCopyAssign(rtClass)
                               : string.Empty;
                case "EventWrappersMoveAssign":
                    return rtClass.Events.Count > 0
                               ? Event.WrappersMoveAssign(rtClass)
                               : string.Empty;
                case "EventWrappersMoveAssignRebind":
                    return rtClass.Events.Count > 0
                               ? Event.WrappersMoveAssignRebind(rtClass)
                               : string.Empty;
                case "EventFields":
                    return rtClass.Events.Count > 0
                               ? Event.Fields(rtClass)
                               : string.Empty;
                case "ForwardDeclareMuiWindowPtr":
                    const string FORWARD_DECLARE = "ForwardDeclareMuiWindow";

                    if (RtFile.AttributeInfo.CustomFlags.TryGet(FORWARD_DECLARE, out bool isForwardDeclare) && isForwardDeclare)
                    {
                        StringBuilder sb = new StringBuilder();
                        sb.AppendLine(MUI_NAMESPACE_START);
                        sb.AppendLine();
                        sb.AppendLine("class WindowPtr;");
                        sb.AppendLine();
                        sb.AppendLine(MUI_NAMESPACE_END);
                        return sb.ToString();
                    }

                    return string.Empty;
                case "TemplatedUsing":
                    if (RtFile.AttributeInfo.PtrMappings.TryGet(rtClass.Type.Name, out ISmartPtr ptr))
                    {
                        if (ptr.IsTemplated && ptr.HasDefaultAlias)
                        {
                            string aliasName = !string.IsNullOrEmpty(ptr.DefaultAliasName)
                                ? ptr.DefaultAliasName
                                : rtClass.Type.NonInterfaceName;

                            if (rtClass.Type.GenericArguments?.Count > 0)
                            {
                                return $"template <{string.Join(", ", rtClass.Type.GenericArguments.Select(arg => $"typename {arg.Name}"))}>{Environment.NewLine}"+
                                       $"using {aliasName} = {rtClass.Type.Wrapper.NameQualified};";
                            }

                            return $"using {aliasName} = {rtClass.Type.Wrapper.NameQualified}<>;";
                        }
                    }
                    return "";
                case "ThisHeaderName":
                    return GetHeaderFromType(rtClass.Type, rtClass, false).Replace("#include ", "");
                case "CustomExtensionsHeader":
                    return GetHeaderFromType(rtClass.Type, rtClass, false);
            }

            return base.GetFileVariable(rtClass, variable);
        }

        private static string GetTemplateArguments(IRTInterface rtClass, bool addEndSeparator = true)
        {
            if (rtClass.Type.GenericArguments == null)
            {
                return "";
            }

            string interfaceTemplateArgs = string.Join(", ", rtClass.Type.GenericArguments.Select(arg => $"typename {arg.Name}"));
            if (addEndSeparator && !string.IsNullOrEmpty(interfaceTemplateArgs))
            {
                interfaceTemplateArgs += ", ";
            }

            return interfaceTemplateArgs;
        }

        private string GenerateUiControlMembers(IRTInterface rtClass)
        {
            using (FileStream template = File.Open(Utility.GetTemplate("cpp.control.template"), FileMode.Open))
            {
                return RenderTemplate(rtClass, template, GetFileVariable);
            }
        }

        protected override string GetMethodVariable(IMethod method, string variable)
        {
            return GetCppMethodVariable(method, variable);
        }

        internal static string GetCppMethodVariable(IMethod method, string variable)
        {
            switch (variable)
            {
                case "CallingConvention" when !string.IsNullOrEmpty(method.CallingConvention):
                {
                    if (method.CallingConvention.TrimStart('_', ' ') == "stdcall")
                    {
                        return CALLING_CONVENTION_MACRO;
                    }

                    return method.CallingConvention;
                }
                case "ConstMethod":
                    if (method.Modifiers.Contains("const"))
                    {
                        return " const";
                    }
                    return "";
                default:
                    return null;
            }
        }

        private string GenerateDocAttributes(IOverload overload, IEnumerable<IDocTag> tags, int ident)
        {
            StringBuilder sb = new StringBuilder();

            foreach (IDocTag tag in tags)
            {
                switch (tag.TagType)
                {
                    case TagType.Brief:
                        sb.Append(' ', ident);
                        sb.Append($"{tag.RawText}");
                        break;
                    case TagType.Param:
                        IDocParam param = (IDocParam) tag;
                        if (param.ParamName == overload?.GetLastByRefArgument()?.Name)
                        {
                            sb.Append(ConvertDocAttribute("returns", param));
                        }
                        else
                        {
                            sb.Append(' ', ident);
                            sb.Append($"* {tag.RawText}");
                        }
                        break;
                    case TagType.ParamRef:
                    case TagType.Private:
                        return tag.RawText;
                    case TagType.RetVal:
                        var retVal = (IDocRetVal) tag;
                        if (retVal.ReturnValue.StartsWith(ERROR_CODE_PREFIX))
                        {
                            sb.Append(ConvertDocAttribute("throws", retVal));
                            break;
                        }
                        else if (retVal.ReturnValue == "OPENDAQ_IGNORED")
                        {
                            sb.Append(ConvertDocAttribute("returns nullptr", retVal));
                            break;
                        }
                        goto case TagType.Unknown;
                    case TagType.Block:
                    {
                        if (!tag.RawText.StartsWith("*", StringComparison.Ordinal))
                        {
                            sb.Append($" * {tag.RawText + Environment.NewLine}");
                        }
                        else
                        {
                            sb.Append($" {tag.RawText}");
                        }
                        break;
                    }
                    case TagType.Description:
                        var description = (IDocDescription) tag;
                        foreach (IDocLine docLine in description.Lines)
                        {
                            sb.Append(' ', ident);
                            sb.Append("* ");
                            sb.AppendLine(docLine.FullText);
                        }
                        break;
                    case TagType.Unknown:
                    case TagType.Throws:
                        sb.Append(' ', ident);
                        sb.AppendLine($"* {tag.RawText}");
                        break;
                }

                sb.TrimTrailingNewLine();
                sb.AppendLine();
            }

            return sb.ToString();
        }

        private string ConvertDocAttribute(string tagName, IDocAttribute tag)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("     * ");
            sb.Append($"@{tagName} ");

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
                    sb.Append("     * ");
                    sb.Append(line.FullText);
                }
            }

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
            sb.AppendLine("/*!");

            var tags = new List<IDocTag>();
            tags.AddRange(doc.Tags);

            if (doc.Brief != null)
            {
                tags.Insert(doc.Brief.TagIndex, doc.Brief);
            }

            if (doc.Description != null)
            {
                tags.Insert(doc.Description.TagIndex, doc.Description);
            }

            sb.Append(GenerateDocAttributes(overload, tags, 5));

            sb.Append(' ', 5);
            sb.Append("*/");

            return sb.ToString();
        }

        private string GenerateWrapperTypeDoc(IDocComment doc)
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine("/*!");

            var tags = new List<IDocTag>();
            tags.AddRange(doc.Tags);

            if (doc.Brief != null)
            {
                tags.Insert(doc.Brief.TagIndex, doc.Brief);
            }

            if (doc.Description != null)
            {
                tags.Insert(doc.Description.TagIndex, doc.Description);
            }

            sb.Append(GenerateDocAttributes(null, tags, 1));

            sb.Append(' ', 1);
            sb.AppendLine("*/");

            return sb.ToString();
        }

        protected override string GetMethodWrapperVariable(IOverload overload, string variable)
        {
            switch (variable)
            {
                case "DocComment":
                    return overload.Method.Documentation != null
                               ? GenerateWrapperDoc(overload)
                               : "";
                case "ExitReturnArgName":
                {
                    IArgument arg = overload.GetLastByRefArgument();
                    if (arg != null && arg.Type.Name == "IEvent")
                    {
                        return GenerateEventType(arg, true);
                    }

                    return GetMethodWrapperVariableInternal(overload, variable);
                }
                case "ReturnTypePtr":
                {
                    IArgument arg = overload.GetLastByRefArgument();
                    if (arg != null && arg.Type.Name == "IEvent")
                    {
                        return GenerateEventType(arg, false);
                    }
                    goto default;
                }
                default:
                    return GetCppMethodWrapperVariable(overload, variable, this);
            }
        }

        private string GenerateEventType(IArgument arg, bool create)
        {
            if (!RtFile.TypeAliases.TryGetValue(arg.Type.UnmappedName, out ITypeName type))
            {
                type = arg.Type;
            }

            if (type.GenericArguments == null)
            {
                throw new GeneratorException("Event is missing Sender and EventArgs type definitions!");
            }

            string returnType = type.Wrapper.NameQualified;
            string genericArgs = string.Join($"{RtFile.AttributeInfo.ParameterSeparator} ", type.GenericArguments.Select(g =>
            {
                return g.Wrapper.NameFull;
            }));

            return create
                ? $"{returnType}<{genericArgs}>({arg.Name})"
                : $"{returnType}<{genericArgs}>";
        }

        internal static string GetCppMethodWrapperVariable(IOverload overload, string variable, BaseGenerator generator)
        {
            switch (variable)
            {
                case "ReturnTypeIntf":
                {
                    IArgument arg = overload.GetLastByRefArgument();
                    if (arg == null)
                    {
                        return string.Empty;
                    }

                    if (!generator.RtFile.TypeAliases.TryGetValue(arg.Type.UnmappedName, out ITypeName argType))
                    {
                        argType = arg.Type;
                    }

                    if (!argType.Flags.IsValueType)
                    {
                        if (argType.Name == "IEvent")
                        {
                            return "EventPtr<>";
                        }

                        return argType.Wrapper.NameFull;
                    }

                    string name = argType.FullName();
                    if (argType.Modifiers == "**")
                    {
                        name += "*";
                    }

                    if (arg.IsConst)
                    {
                        name = "const " + name;
                    }

                    return name;
                }
                case "Arguments":
                    return generator.GetMethodArguments(overload,
                                                        "$ArgTypeQualifiers$$ArgTypeFull$$ArgTypeModifiers$ $ArgName$$ArgDefaultValue$",
                                                        "$ArgTypeQualifiers$$ArgTypeFull$$ArgTypeModifiers$ $ArgName$$ArgDefaultValue$",
                                                        ", ");
                case "ArgumentNames":
                    return generator.GetArgumentNames(overload, ", ", "&");
                default:
                    return null;
            }
        }

        protected override string GetArgumentName(IOverload overload, IArgument argument)
        {
            if (argument.ArrayInfo != null && overload.Type == OverloadType.Helper)
            {
                return $"{argument.Name}.data()";
            }

            return base.GetArgumentName(overload, argument);
        }

        public override string GetArgumentNames(IOverload overload, string separator, string refSymbol)
        {
            if (overload.Type == OverloadType.Helper)
            {
                List<string> argNames = new List<string>(overload.Arguments.Count + 2);
                foreach (IArgument argument in overload.Arguments)
                {
                    argNames.Add($"{(argument.IsOutParam ? refSymbol : "")}{GetArgumentName(overload, argument)}");
                }

                foreach (IArgument argument in overload.Arguments)
                {
                    IArray array = argument.ArrayInfo;
                    if (array?.ExtentType == ArrayExtent.Parameter)
                    {
                        int paramIndex = array.ParameterIndex;
                        if (argNames.Count > paramIndex && argNames[paramIndex] != array.Reference)
                        {
                            argNames.Insert(paramIndex, array.Reference);
                        }

                        if (argNames.Count == paramIndex)
                        {
                            argNames.Add(array.Reference);
                        }
                    }
                }

                return string.Join(separator, argNames);
            }

            return base.GetArgumentNames(overload, separator, refSymbol);
        }

        protected override string GetMethodArgumentVariable(IArgument arg, IOverload method, string variable)
        {
            return GetCppMethodArgumentVariable(arg, variable, Options);
        }

        internal static string GetCppMethodArgumentVariable(IArgument arg, string variable, IGeneratorOptions options)
        {
            switch (variable)
            {
                case "ArgTypeFull":
                    if (arg.Type != null)
                    {
                        if (options.GenerateWrapper && !arg.Type.Flags.IsValueType)
                        {
                            return arg.Type.Wrapper.NameFull;
                        }

                        if (!options.GenerateWrapper)
                        {
                            string argType = arg.Type.FullName();
                            if (!arg.Type.Flags.IsValueType)
                            {
                                argType += "*";
                            }

                            return argType;
                        }
                    }
                    return null;
                case "ArgTypeModifiers":
                    if (options.GenerateWrapper && arg.Type != null)
                    {
                        if (arg.Type.Name == "void")
                        {
                            return "";
                        }

                        if (arg.ArrayInfo != null)
                        {
                            return arg.Type.Modifiers;
                        }

                        if (arg.IsOutParam || !arg.Type.Flags.IsValueType)
                        {
                            return "&";
                        }
                        return "";
                    }
                    return null;
                case "ArgTypeQualifiers":
                    if (options.GenerateWrapper &&
                        !arg.IsOutParam &&
                        !arg.Type.Flags.IsValueType &&
                        arg.Type.Name != "void")
                    {
                        return "const ";
                    }
                    return null;
                case "ArgDefaultValue":
                    return arg.DefaultValue != null
                        ? $" = {arg.DefaultValue}"
                        : "";
                default:
                    return null;
            }
        }

        protected override string GetIncludes(IRTFile rtFile)
        {
            HashSet<string> headers = new HashSet<string>();

            FileFeatures features = rtFile.AdditionalFeatures;
            if (features.HasFlag(FileFeatures.Events))
            {
                AddInclude(headers, "<mui/event.h>");
            }

            if (features.HasFlag(FileFeatures.Controls))
            {
                AddInclude(headers, "<mui/control_factory_ptr.h>");

                if (!rtFile.AttributeInfo.CustomFlags.Has("ForwardDeclareMuiWindow"))
                {
                    AddInclude(headers, "<mui/layout/window_ptr.h>");
                }
            }

            if (features.HasFlag(FileFeatures.Span))
            {
                AddInclude(headers, "<coretypes/span.h>");
            }

            rtFile.AdditionalFeatures = features;

            foreach (string header in rtFile.Includes)
            {
                AddInclude(headers, header);
            }

            foreach (string header in rtFile.AttributeInfo.AdditionalHeaders)
            {
                AddInclude(headers, header);
            }

            if (RtFile.CurrentClass.BaseType.Wrapper.BaseNameFull != rtFile.AttributeInfo.GetDefaultBasePtrFull())
            {
                string header = rtFile.CurrentClass.BaseType.Wrapper.IncludeName;
                string include = header.StartsWith("<") || header.StartsWith("\"")
                                     ? $"#include {header}"
                                     : string.Format(INCLUDE_FORMAT, header + ".h");

                headers.Add(include);
            }

            if (Options.GenerateWrapper)
            {
                foreach (string header in GetImplicitHeaderIncludes(headers, rtFile.CurrentClass))
                {
                    headers.Add(header);
                }
            }

            const int AVG_HEADER_INCLUDE_LENGHT = 40;

            StringBuilder headerIncludes = new StringBuilder(headers.Count * AVG_HEADER_INCLUDE_LENGHT);
            foreach (string header in headers)
            {
                headerIncludes.AppendLine(header);
            }

            return headerIncludes.ToString();
        }

        private void AddInclude(HashSet<string> headers, string header)
        {
            headers.Add(header.StartsWith("<") || header.StartsWith("\"")
                            ? $"#include {header}"
                            : string.Format(INCLUDE_FORMAT, header));
        }

        private string[] GetImplicitHeaderIncludes(HashSet<string> headers, IRTInterface rtClass)
        {
            // Base interface unit
            string baseHeader = GetHeaderFromType(rtClass.BaseType, rtClass);

            if (!string.IsNullOrEmpty(baseHeader))
            {
                headers.Add(baseHeader);
            }

            GetHeaderMethodArguments(headers, rtClass);

            string[] headersArray = new string[headers.Count];
            headers.CopyTo(headersArray);

            return headersArray;
        }

        private string GetHeaderFromType(ITypeName type, IRTInterface rtClass, bool ignoreSelf = true)
        {
            if (RtFile.TypeAliases.TryGetValue(type.UnmappedName, out ITypeName aliased))
            {
                return GetHeaderFromType(aliased, rtClass);
            }

            if (type.Flags.IsValueType)
            {
                return null;
            }

            if (type.Namespace != null && type.Namespace.Raw == $"{AttributeInfo.DEFAULT_CORE_NAMESPACE}::")
            {
                type.Namespace.Raw = AttributeInfo.DEFAULT_CORE_NAMESPACE;
            }

            bool isSelf = type.FullName(false) == rtClass.Type.FullName(false);
            if (ignoreSelf && isSelf)
            {
                return null;
            }

            string unmappedName = type.UnmappedName;

            // Exclude void and enums defined in this file from the includes list
            if (unmappedName == "void" || RtFile.Enums.Any(e => e.Name == unmappedName))
            {
                return null;
            }

            string libraryName = type.LibraryName;
            if (RtFile.AttributeInfo.IsCoreType(unmappedName))
            {
                libraryName = "CoreTypes";
            }

            string ptrIncludeName = type.Wrapper.IncludeName;
            if (string.IsNullOrEmpty(ptrIncludeName))
            {
                return null;
            }

            bool isAngleBracket = ptrIncludeName.IndexOf('<') != -1;

            if (isSelf)
            {
                ptrIncludeName += ".custom";
            }

            if (!string.IsNullOrEmpty(libraryName) && !isAngleBracket && ptrIncludeName.IndexOfAny(new[] { '/', '\\' }) == -1)
            {
                return $"#include <{libraryName.ToLowerInvariant()}/{ptrIncludeName}.h>";
            }

            if (isAngleBracket)
            {
                return "#include " + ptrIncludeName;
            }
            
            return "#include \"" + ptrIncludeName + ".h\"";
        }

        private void GetHeaderMethodArguments(HashSet<string> headers, IRTInterface rtClass)
        {
            foreach (IMethod method in rtClass.Methods)
            {
                if (Options.GenerateWrapper && method.IsIgnored.HasFlag(GeneratorType.Wrapper))
                {
                    continue;
                }

                foreach (IArgument argument in method.Arguments)
                {
                    GetHeaderFromArgument(headers, argument, rtClass);
                }
            }
        }

        private void GetHeaderFromArgument(ISet<string> headers, IArgument argument, IRTInterface rtClass)
        {
            ITypeName argumentType = argument.Type;

            string argUnit = GetHeaderFromType(argumentType, rtClass);
            if (!string.IsNullOrEmpty(argUnit))
            {
                headers.Add(argUnit);
            }

            if (argumentType.GenericArguments != null)
            {
                foreach (ITypeName templateType in argumentType.GenericArguments)
                {
                    string templateUnit = GetHeaderFromType(templateType, rtClass);
                    if (!string.IsNullOrEmpty(templateUnit))
                    {
                        headers.Add(templateUnit);
                    }
                }
            }
        }
    }
}
