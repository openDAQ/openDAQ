using RTGen.Exceptions;
using RTGen.Generation;
using RTGen.Interfaces;
using RTGen.Types;
using RTGen.Util;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace RTGen.C.Generators
{
    class CGenerator : TemplateGenerator
    {
        protected enum GeneratorType
        {
            Header = 0,
            Source = 1,
        }

        protected enum ArgsListType
        {
            MethodDeclaration,
            ArgsForwarding
        }

        protected static ISet<string> ForbiddenTypes => new HashSet<string>
        {
            "IntfID",
            "ComplexFloat64"
        };

        protected GeneratorType _generatorType = GeneratorType.Header;
        protected ISet<string> _typesToDeclare = new HashSet<string>();
        protected ISet<string> _methodNamesToCommentOut = new HashSet<string>();

        //Overriden

        public override IVersionInfo Version => new VersionInfo
        {
            Major = 0,
            Minor = 1,
            Patch = 0
        };

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

                if (base.Options.LibraryInfo == null)
                {
                    base.Options.LibraryInfo = new LibraryInfo();
                }
                if (string.IsNullOrEmpty(base.Options.LibraryInfo.OutputName))
                {
                    base.Options.LibraryInfo.OutputName = "copendaq";
                }

                if (base.Options.GeneratedExtension == ".h")
                {
                    _generatorType = GeneratorType.Header;
                }
                else if (base.Options.GeneratedExtension == ".cpp")
                {
                    _generatorType = GeneratorType.Source;
                }
            }
        }

        protected override StringBuilder WriteMethods(IRTInterface rtClass, string baseTemplatePath)
        {
            return new StringBuilder();
        }

        protected override string GetMethodArgumentVariable(IArgument arg, IOverload overload, string variable)
        {
            if (variable == "ArgTypeNameNonInterface")
            {
                return arg.Type.NonInterfaceName;
            }
            return null;
        }

        protected override string GetIncludes(IRTFile rtFile)
        {
            StringBuilder sb = new StringBuilder();

            if (_generatorType == GeneratorType.Header)
            {
                string common = "ccommon";

                sb.AppendLine($"#include \"{common}.h\"");
            }
            else if (_generatorType == GeneratorType.Source)
            {
                string lib = Options.LibraryInfo.Name;
                string file = Options.Filename ?? "";
                var header = (lib ?? "") + (String.IsNullOrEmpty(lib) ? "" : "/") + (file ?? "") + ".h";
                sb.AppendLine($"#include \"{header}\"");
                sb.AppendLine();
                sb.AppendLine("#include <opendaq/opendaq.h>");
            }
            return sb.ToString();
        }

        public override void GenerateFile(string templatePath)
        {
            string headerTemplatePath = GetHeaderTemplatePath(templatePath);
            if (Log.Verbose)
            {
                Log.Info($"Generating {Options.Language} bindings for file: {RtFile.SourceFileName} extension: {Options.GeneratedExtension}");
            }

            try
            {
                if (_generatorType == GeneratorType.Source)
                {
                    string tmpSourcePath = Path.GetTempFileName();
                    string outputSourcePath = GetOutputPath(".cpp");
                    GenerateSource(templatePath, tmpSourcePath);
                    File.Copy(tmpSourcePath, outputSourcePath, true);
                }
                else
                {
                    string tmpHeaderPath = Path.GetTempFileName();
                    string outputHeaderPath = GetOutputPath();
                    GenerateHeader(headerTemplatePath, tmpHeaderPath);
                    File.Copy(tmpHeaderPath, outputHeaderPath, true);
                }
            }
            catch (Exception e)
            {
                throw new GeneratorException($"Failed to generate {Options.Language} bindings for file: {RtFile.SourceFileName} with error: {e.Message}");
            }
        }

        //Overriden end

        protected string GetIntfIDDeclaration()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine(base.Indentation + $"EXPORTED extern const IntfID {RtFile.CurrentClass.Type.NonInterfaceName.ToLowerSnakeCase().ToUpper()}_INTF_ID;");
            return sb.ToString();
        }

        protected string GetIntfIDDefinition()
        {
            StringBuilder sb = new StringBuilder();
            string ns = RtFile.CurrentClass.Type.Namespace.ToString();
            string iface = RtFile.CurrentClass.Type.Name;

            sb.Append($"const IntfID {RtFile.CurrentClass.Type.NonInterfaceName.ToLowerSnakeCase().ToUpper()}_INTF_ID = ");
            sb.AppendLine($"{{ {ns}::{iface}::Id.Data1, {ns}::{iface}::Id.Data2, {ns}::{iface}::Id.Data3, {ns}::{iface}::Id.Data4_UInt64 }};");
            return sb.ToString();
        }

        protected string GetTypedefs()
        {
            StringBuilder sb = new StringBuilder();
            foreach (var type in _typesToDeclare)
            {
                if (type == "BaseObject")
                {
                    continue;
                }
                sb.AppendLine(base.Indentation + $"typedef struct {type} {type};");
            }
            return sb.ToString();
        }

        protected string GetOutputPath(string overridenExtension = null)
        {
            return String.IsNullOrWhiteSpace(overridenExtension) ? GetOutputFilePath() : Path.ChangeExtension(GetOutputFilePath(), overridenExtension);
        }

        protected string GetHeaderTemplatePath(string templatePath)
        {
            return Path.Combine(Path.GetDirectoryName(templatePath), "c.header.template");
        }

        protected string ArgumentListToString(IEnumerable<IArgument> args, string separator)
        {
            return String.Join(separator, args.Select(arg => arg.Name));
        }

        protected string ArgumentsToString<T>(T methodOrFactory, ArgsListType listType)
        {
            IRTFactory factory = methodOrFactory as IRTFactory;
            IMethod method = methodOrFactory as IMethod;
            IOverload overload = method != null ? method.Overloads[0] : factory.ToOverload();

            IList<IArgument> args = overload.Arguments.ToList();

            //adding self pointer to the method declaration
            if (listType == ArgsListType.MethodDeclaration && method != null)
            {
                IArgument self = new Argument(RtFile.CurrentClass.Type, "self");
                self.Type.Modifiers = "*";
                args.Insert(0, self);
            }

            StringBuilder sb = new StringBuilder();

            for (int i = 0; i < args.Count; i++)
            {
                IArgument arg = args[i];

                //TODO: should be removed later
                //as some parts of the bindings are not yet implemented
                //we need to comment out the methods that use non implemented types
                if (ForbiddenTypes.Contains(arg.Type.NonInterfaceName))
                {
                    _methodNamesToCommentOut.Add(overload.Method.Name);
                }
                else if (!arg.Type.Flags.IsValueType)
                {
                    //filling types for typedefs
                    if (arg.Type.Name != "void") _typesToDeclare.Add(arg.Type.NonInterfaceName);
                }

                if (listType == ArgsListType.MethodDeclaration)
                {
                    sb.Append($"{arg.Type.NonInterfaceName}{arg.Type.Modifiers} {arg.Name}");
                }
                else
                {
                    //skipping self pointer in factory call
                    if (factory != null && i == 0)
                    {
                        if (i != args.Count - 1) sb.Append(", ");
                        continue;
                    }

                    if (arg.Type.Flags.IsValueType || arg.Type.Name == "void")
                    {
                        if (arg.Type.Name == "CoreType") //special CoreType handling
                        {
                            if (String.IsNullOrEmpty(arg.Type.Modifiers))
                            {
                                sb.Append($"static_cast<{arg.Type.Namespace}::{arg.Type.Name}>({arg.Name})");
                            }
                            else
                            {
                                sb.Append($"reinterpret_cast<{arg.Type.Namespace}::{arg.Type.Name}{arg.Type.Modifiers}>({arg.Name})");
                            }
                        }
                        else
                        {
                            sb.Append(arg.Name);
                        }
                    }
                    else
                    {
                        sb.Append($"reinterpret_cast<{arg.Type.Namespace}::{arg.Type.Name}{arg.Type.Modifiers}>({arg.Name})");
                    }
                }
                if (i != args.Count - 1)
                {
                    sb.Append(", ");
                }
            }
            return sb.ToString();
        }

        protected void GenerateHeader(string templatePath, string outputPath)
        {
            string methodTemplatePath = Path.Combine(Path.GetDirectoryName(templatePath), Path.GetFileNameWithoutExtension(templatePath) + ".method.template");

            SetVariables(RtFile.CurrentClass, templatePath);
            StringBuilder methods = GenerateMethodsHeader(methodTemplatePath);
            methods.TrimTrailingNewLines();

            Variables["Methods"] = methods.ToString();
            Variables["headers"] = GetIncludes(RtFile);
            Variables["typedefs"] = GetTypedefs();
            Variables["intfid_declaration"] = GetIntfIDDeclaration();

            GenerateOutput(RtFile.CurrentClass, templatePath, outputPath);
        }
        protected void GenerateSource(string templatePath, string outputPath)
        {
            string methodTemplatePath = Path.Combine(Path.GetDirectoryName(templatePath), Path.GetFileNameWithoutExtension(templatePath) + ".method.template");

            SetVariables(RtFile.CurrentClass, templatePath);
            StringBuilder methods = GenerateMethodsSource(methodTemplatePath);
            methods.TrimTrailingNewLines();

            Variables["Methods"] = methods.ToString();
            Variables["headers"] = GetIncludes(RtFile);
            Variables["intfid_definition"] = GetIntfIDDefinition();

            GenerateOutput(RtFile.CurrentClass, templatePath, outputPath);
        }

        protected void GenerateOutput(IRTInterface rtClass, string templatePath, string outputPath)
        {
            StreamReader template = null;
            try
            {
                FileInfo info = new FileInfo(templatePath);
                FileInfo outputInfo = new FileInfo(outputPath);
                template = new StreamReader(info.Open(FileMode.Open, FileAccess.Read, FileShare.Read));

                using (StreamWriter output = new StreamWriter(outputInfo.Open(FileMode.Create, FileAccess.Write, FileShare.None)))
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

                            var str = GetGlobalVariable(rtClass, templatePath, variable);
                            return str;
                        });
                        output.WriteLine(outputLine);
                    }
                }
                template.Close();
            }
            catch (Exception)
            {
                template?.Dispose();
                throw;
            }
        }

        protected string ReplaceVariable<T>(string variable, GeneratorType generatorType, IRTInterface iface, T methodOrFactory, string templatePath)
        {
            IMethod method = methodOrFactory as IMethod;
            IRTFactory factory = methodOrFactory as IRTFactory;
            IOverload overload = method != null ? method.Overloads[0] : factory.ToOverload();

            //TODO: merge these two branches
            if (generatorType == GeneratorType.Header)
            {
                switch (variable)
                {
                    case "Name":
                        return method != null ? method.Name : factory.Name ?? "";
                    case "ReturnType":
                        return method != null ? method.ReturnType.NonInterfaceName : "ErrCode";
                    case "NonInterfaceType":
                        string typeName = method != null ? iface.Type.NonInterfaceName : factory.InterfaceName ?? "";
                        if (factory != null && !String.IsNullOrEmpty(typeName))
                        {
                            typeName = typeName.Remove(0, 1);
                        }
                        return typeName;
                    case "Arguments":
                        return ArgumentsToString(methodOrFactory, ArgsListType.MethodDeclaration);
                    default:
                        LogIgnoredVariable(variable, templatePath);
                        return string.Empty;
                }
            }
            else if (generatorType == GeneratorType.Source)
            {
                switch (variable)
                {
                    case "Name":
                        return method != null ? method.Name : factory.Name ?? "";
                    case "ReturnType":
                        return method != null ? method.ReturnType.NonInterfaceName : "ErrCode";
                    case "NonInterfaceType":
                        string typeName = method != null ? iface.Type.NonInterfaceName : factory.InterfaceName ?? "";
                        if (factory != null && !String.IsNullOrEmpty(typeName))
                        {
                            typeName = typeName.Remove(0, 1);
                        }
                        return typeName;
                    case "ArgTypeFull":
                        return iface.Type.FullName();
                    case "FactoryName":
                        return iface.Type.Namespace.ToString() + "::" + overload.Method.Name;
                    case "Arguments":
                        return ArgumentsToString(methodOrFactory, ArgsListType.MethodDeclaration);
                    case "ArgumentsForwarding":
                        return ArgumentsToString(methodOrFactory, ArgsListType.ArgsForwarding);
                    default:
                        LogIgnoredVariable(variable, templatePath);
                        return string.Empty;
                }
            }
            return String.Empty;
        }

        protected StringBuilder GenerateMethodsHeader(string templatePath)
        {
            IRTInterface rtClass = RtFile.CurrentClass;
            StringBuilder methods = new StringBuilder();
            if (!File.Exists(templatePath))
            {
                Log.Warning($"Method template {templatePath} does not exist");
                return null;
            }

            string[] methodTemplate = File.ReadAllLines(templatePath);

            string methodDeclarationTemplate = methodTemplate[0];
            string factoryDeclarationTemplate = methodTemplate[1];

            foreach (IMethod method in rtClass.Methods)
            {
                string template = methodDeclarationTemplate;

                if (!HandleMemberMethod(rtClass, method))
                {
                    continue;
                }

                string generatedMethod = ReplacementRegex.Replace(template, m =>
                {
                    string variable = m.Groups[1].Value;
                    return ReplaceVariable(variable, GeneratorType.Header, rtClass, method, templatePath);
                });

                bool isCommentedOut = _methodNamesToCommentOut.Contains(method.Name);
                if (isCommentedOut) methods.AppendLine("/*");
                methods.AppendLine(base.Indentation + generatedMethod);
                if (isCommentedOut) methods.AppendLine("*/");
            }

            foreach (IRTFactory factory in RtFile.Factories)
            {
                IOverload factoryMethod = factory.ToOverload();
                string generatedFactory = ReplacementRegex.Replace(factoryDeclarationTemplate, m =>
                {
                    string variable = m.Groups[1].Value;
                    return ReplaceVariable(variable, GeneratorType.Header, rtClass, factory, templatePath);
                });

                bool isCommentedOut = _methodNamesToCommentOut.Contains(factory.Name);
                if (isCommentedOut) methods.AppendLine("/*");
                methods.AppendLine(base.Indentation + generatedFactory);
                if (isCommentedOut) methods.AppendLine("*/");
            }

            return methods;
        }

        protected StringBuilder GenerateMethodsSource(string templatePath)
        {
            IRTInterface rtClass = RtFile.CurrentClass;
            StringBuilder methods = new StringBuilder();
            if (!File.Exists(templatePath))
            {
                Log.Warning($"Method template {templatePath} does not exist");
                return null;
            }

            string[] methodTemplate = File.ReadAllLines(templatePath);

            string methodDefinitionTemplate = methodTemplate[0];
            string factoryDefinitionTemplate = methodTemplate[1];
            string implMethodTemplate = methodTemplate[2];
            string pointerDeclarationTemplate = methodTemplate[3];
            string factoryCallTemplate = methodTemplate[4];
            string pointerCastTemplate = methodTemplate[5];
            string returnZeroTemplate = methodTemplate[6];
            string returnErrorTemplate = methodTemplate[7];

            foreach (IMethod method in rtClass.Methods)
            {
                if (!HandleMemberMethod(rtClass, method))
                {
                    continue;
                }

                string generatedMethod = ReplacementRegex.Replace(methodDefinitionTemplate, m =>
                {
                    string variable = m.Groups[1].Value;

                    string customVariable = GetMethodVariable(method, variable);
                    if (customVariable != null)
                    {
                        return customVariable;
                    }

                    return ReplaceVariable(variable, GeneratorType.Source, rtClass, method, templatePath);
                });

                string generatedMethodImpl = ReplacementRegex.Replace(implMethodTemplate, m =>
                {
                    string variable = m.Groups[1].Value;
                    string customVariable = GetMethodVariable(method, variable);
                    if (customVariable != null)
                    {
                        return customVariable;
                    }
                    return ReplaceVariable(variable, GeneratorType.Source, rtClass, method, templatePath);
                });

                bool isCommentedOut = _methodNamesToCommentOut.Contains(method.Name);

                if (isCommentedOut) methods.AppendLine("/*");
                methods.AppendLine(generatedMethod);
                methods.AppendLine("{");
                methods.AppendLine(base.Indentation + generatedMethodImpl);
                methods.AppendLine("}");
                if (isCommentedOut) methods.AppendLine("*/");
                methods.AppendLine();
            }

            foreach (IRTFactory factory in RtFile.Factories)
            {
                IOverload factoryMethod = factory.ToOverload();

                string generatedFactory = ReplacementRegex.Replace(factoryDefinitionTemplate, m =>
                    ReplaceVariable(m.Groups[1].Value, GeneratorType.Source, rtClass, factory, templatePath)
                );

                string objectPointer = ReplacementRegex.Replace(pointerDeclarationTemplate, m =>
                    ReplaceVariable(m.Groups[1].Value, GeneratorType.Source, rtClass, factory, templatePath)
                );

                string factoryCall = ReplacementRegex.Replace(factoryCallTemplate, m =>
                    ReplaceVariable(m.Groups[1].Value, GeneratorType.Source, rtClass, factory, templatePath)
                );

                string objectPointerCast = ReplacementRegex.Replace(pointerCastTemplate, m =>
                    ReplaceVariable(m.Groups[1].Value, GeneratorType.Source, rtClass, factory, templatePath)
                );

                bool isCommentedOut = _methodNamesToCommentOut.Contains(factory.Name);

                if (isCommentedOut) methods.AppendLine("/*");
                methods.AppendLine(generatedFactory);
                methods.AppendLine("{");
                methods.AppendLine(base.Indentation + objectPointer);
                methods.AppendLine(base.Indentation + factoryCall);
                methods.AppendLine(base.Indentation + objectPointerCast);
                methods.AppendLine(base.Indentation + returnErrorTemplate);
                methods.AppendLine("}");
                if (isCommentedOut) methods.AppendLine("*/");
                methods.AppendLine();
            }
            return methods;
        }
    }
}

