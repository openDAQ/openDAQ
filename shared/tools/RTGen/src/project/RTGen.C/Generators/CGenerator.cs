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
        enum GeneratorType
        {
            Header = 0,
            Source = 1,
        }

        private GeneratorType _generatorType = GeneratorType.Header;

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

        protected override void SetCustomVariables()
        {
            Variables["typedefs"] = GetTypedefs();
        }

        protected override StringBuilder WriteMethods(IRTInterface rtClass, string baseTemplatePath)
        {
            return new StringBuilder();
        }

        protected override string GetMethodArgumentVariable(IArgument arg, IOverload overload, string variable)
        {
            if(variable == "ArgTypeNameNonInterface")
            {
                return arg.Type.NonInterfaceName;
            }
            return null;
        }

        protected override string GetIncludes(IRTFile rtFile)
        {
            StringBuilder sb = new StringBuilder();

            if (this._generatorType == GeneratorType.Header)
            {
                string common = "ccommon";
                string lib = Options.LibraryInfo.OutputName;

                sb.AppendLine($"#include <{lib}/{lib}.h>");
                sb.AppendLine($"#include <{lib}/{common}.h>");
            }
            else if (this._generatorType == GeneratorType.Source)
            {
                sb.AppendLine($"#include \"{Options.Filename}.h\"");
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

        protected string GetTypedefs()
        {
            StringBuilder sb = new StringBuilder();
            string type = RtFile.CurrentClass.Type.NonInterfaceName;
            sb.AppendLine($"struct {type};");
            sb.AppendLine($"typedef struct {type} {type};");
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

        protected void GenerateHeader(string templatePath, string outputPath)
        {
            string methodTemplatePath = Path.Combine(Path.GetDirectoryName(templatePath), Path.GetFileNameWithoutExtension(templatePath) + ".method.template");

            SetVariables(RtFile.CurrentClass, templatePath);
            StringBuilder methods = GenerateMethodsHeader(methodTemplatePath);
            methods.TrimTrailingNewLines();

            Variables["Methods"] = methods.ToString();
            Variables["headers"] = GetIncludes(RtFile);

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

            GenerateOutput(RtFile.CurrentClass, templatePath, outputPath);
        }

        private void GenerateOutput(IRTInterface rtClass, string templatePath, string outputPath)
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

        string ReplaceVariable<T>(string variable, GeneratorType generatorType, IRTInterface iface, T methodOrFactory, string templatePath, string outArgTemplate, string inArgTemplate, string separator)
        {
            IMethod method = methodOrFactory as IMethod;
            IRTFactory factory = methodOrFactory as IRTFactory;
            IOverload overload = method != null ? method.Overloads[0] : factory.ToOverload();

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
                        return GetMethodArguments(overload, outArgTemplate, inArgTemplate, separator);
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
                        return GetMethodArguments(overload, outArgTemplate, inArgTemplate, separator);
                    case "ArgumentsImpl":
                        var args = method != null ? overload.Arguments : overload.Arguments.Skip(1);
                        return ArgumentListToString(args, separator);
                    default:
                        LogIgnoredVariable(variable, templatePath);
                        return string.Empty;
                }
            }
            return String.Empty;
        }

        StringBuilder GenerateMethodsHeader(string templatePath)
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
            string outArgTemplate = methodTemplate[2];
            string inArgTemplate = methodTemplate[3];
            string separator = methodTemplate[4];

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
                    return ReplaceVariable(variable, GeneratorType.Header, rtClass, method, templatePath, outArgTemplate, inArgTemplate, separator);
                });
                methods.AppendLine(base.Indentation + generatedMethod);
            }

            foreach (IRTFactory factory in RtFile.Factories)
            {
                IOverload factoryMethod = factory.ToOverload();
                string generatedFactory = ReplacementRegex.Replace(factoryDeclarationTemplate, m =>
                {
                    string variable = m.Groups[1].Value;
                    return ReplaceVariable(variable, GeneratorType.Header, rtClass, factory, templatePath, outArgTemplate, inArgTemplate, separator);
                });
                methods.AppendLine(base.Indentation + generatedFactory);
            }

            return methods;
        }

        StringBuilder GenerateMethodsSource(string templatePath)
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
            string implFactoryTemplate = methodTemplate[3];
            string returnTemplate = methodTemplate[4];
            string outArgTemplate = methodTemplate[5];
            string inArgTemplate = methodTemplate[6];
            string separator = methodTemplate[7];

            foreach (IMethod method in rtClass.Methods)
            {
                string template = methodDefinitionTemplate;

                if (!HandleMemberMethod(rtClass, method))
                {
                    continue;
                }

                string generatedMethod = ReplacementRegex.Replace(template, m =>
                {
                    string variable = m.Groups[1].Value;

                    string customVariable = GetMethodVariable(method, variable);
                    if (customVariable != null)
                    {
                        return customVariable;
                    }

                    return ReplaceVariable(variable, GeneratorType.Source, rtClass, method, templatePath, outArgTemplate, inArgTemplate, separator);
                });
                if (String.IsNullOrEmpty(generatedMethod))
                {
                    continue;
                }

                string generatedMethodImpl = ReplacementRegex.Replace(implMethodTemplate, m =>
                {
                    string variable = m.Groups[1].Value;
                    string customVariable = GetMethodVariable(method, variable);
                    if (customVariable != null)
                    {
                        return customVariable;
                    }
                    return ReplaceVariable(variable, GeneratorType.Source, rtClass, method, templatePath, outArgTemplate, inArgTemplate, separator);
                });
                if (String.IsNullOrEmpty(generatedMethodImpl))
                {
                    continue;
                }

                methods.AppendLine(generatedMethod);
                methods.AppendLine("{");
                methods.AppendLine(base.Indentation + generatedMethodImpl);
                methods.AppendLine("}");
                methods.AppendLine();
            }

            foreach (IRTFactory factory in RtFile.Factories)
            {
                IOverload factoryMethod = factory.ToOverload();

                string generatedFactory = ReplacementRegex.Replace(factoryDefinitionTemplate, m =>
                {
                    string variable = m.Groups[1].Value;
                    return ReplaceVariable(variable, GeneratorType.Source, rtClass, factory, templatePath, outArgTemplate, inArgTemplate, separator);
                });
                if (String.IsNullOrEmpty(generatedFactory))
                {
                    continue;
                }

                string generatedFactoryImpl = ReplacementRegex.Replace(implFactoryTemplate, m =>
                {
                    string variable = m.Groups[1].Value;
                    return ReplaceVariable(variable, GeneratorType.Source, rtClass, factory, templatePath, outArgTemplate, inArgTemplate, separator);
                });
                if (String.IsNullOrEmpty(generatedFactoryImpl))
                {
                    continue;
                }

                methods.AppendLine(generatedFactory);
                methods.AppendLine("{");
                methods.AppendLine(base.Indentation + generatedFactoryImpl);
                methods.AppendLine(base.Indentation + returnTemplate);
                methods.AppendLine("}");
                methods.AppendLine();
            }
            return methods;
        }
    }
}

