using System;
using System.IO;
using System.Linq;
using System.Text;
using RTGen.Generation;
using RTGen.Interfaces;
using RTGen.Types;
using RTGen.Util;

namespace RTGen.Cpp.Generators
{
    sealed class DecoratorGenerator : TemplateGenerator
    {
        private readonly IRTInterface _rtClass;
        private string _templatePath;

        public DecoratorGenerator(IRTInterface rtClass, IGeneratorOptions options)
        {
            Options = (IGeneratorOptions) options.Clone();
            _rtClass = rtClass;
        }

        public override IVersionInfo Version => new VersionInfo
        {
            Major = 2,
            Minor = 0,
            Patch = 0
        };

        protected override string GetMethodVariable(IMethod method, string variable)
        {
            return CppGenerator.GetCppMethodVariable(method, variable);
        }

        protected override string GetFileVariable(IRTInterface rtClass, string variable)
        {
            switch (variable)
            {
                case "PtrName" when rtClass.Type.Flags.IsGeneric:
                    return rtClass.Type.Wrapper.Name + "<>";
                case "BaseDecorator":
                    if (rtClass.BaseType.Name != "IBaseObject")
                    {
                        return rtClass.BaseType.NonInterfaceName + "DecoratorBase";
                    }
                    return "ObjectDecorator";
                default:
                    return null;
            }
        }

        protected override string GetMethodArgumentVariable(IArgument arg, IOverload overload, string variable)
        {
            if (!arg.Type.Flags.IsValueType && variable == "ArgTypeFull")
            {
                return arg.Type.FullName(false);
            }

            return CppGenerator.GetCppMethodArgumentVariable(arg, variable, Options);
        }

        public override string GetArgumentNames(IOverload overload, string separator, string refSymbol)
        {
            return string.Join(separator, overload.Arguments.Select(arg => arg.Name));
        }

        public override void GenerateFile(string templatePath)
        {
            throw new NotImplementedException();
        }

        public void Generate()
        {
            string path = Path.Combine(Options.OutputDir, Options.Filename);
            string headerFile = path + "_decorator.h";

            _templatePath = Utility.GetTemplate("cpp.decorate.template");

            SetVariables(_rtClass, Utility.GetTemplate("cpp.template"));
            SetDecoratorVariables(headerFile);

            StreamReader template = null;
            try
            {
                FileInfo info = new FileInfo(_templatePath);
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

                            string customValue = GetFileVariable(_rtClass, variable);
                            if (customValue != null)
                            {
                                return customValue;
                            }

                            if (Variables.ContainsKey(variable))
                            {
                                return Variables[variable];
                            }

                            LogIgnoredVariable(variable, _templatePath);
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

        public override string Generate(string templatePath)
        {
            throw new NotImplementedException();
        }

        private void SetDecoratorVariables(string headerName)
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

            Variables["SourceFileInclude"] = sourceInclude;

            StringBuilder includes = new StringBuilder();

            includes.AppendLine(
                _rtClass.BaseType.Name != "IBaseObject"
                    ? $"#include <{_rtClass.BaseType.LibraryName}/{_rtClass.BaseType.DefaultIncludeName}_decorator.h>"
                    : "#include <coretypes/object_decorator.h>"
            );

            includes.TrimTrailingNewLines();
            Variables.Add("headers", includes.ToString());
        }

        protected override string GetMethodWrapperVariable(IOverload overload, string variable)
        {
            if (variable == "Override")
            {
                return "override";
            }

            bool backup = Options.GenerateWrapper;
            Options.GenerateWrapper = false;

            string result = CppGenerator.GetCppMethodWrapperVariable(overload, variable, this);

            Options.GenerateWrapper = backup;
            return result;
        }

        protected override StringBuilder WriteMethodWrappers(IRTInterface rtClass, string templatePath)
        {
            StringBuilder methods = new StringBuilder();

            string templateDir = Path.GetDirectoryName(_templatePath) ?? "";
            string methodTemplatePath = Path.Combine(templateDir, Path.GetFileNameWithoutExtension(_templatePath) + ".method.template");

            string methodImplTemplate = File.ReadAllText(methodTemplatePath);

            foreach (IMethod method in rtClass.Methods)
            {
                WriteMethodWrapper(method, methodImplTemplate, methods);
                methods.AppendLine();
            }

            methods.TrimTrailingNewLines();

            return methods;
        }
    }
}
