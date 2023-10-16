using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using RTGen.Exceptions;
using RTGen.Interfaces;
using RTGen.Util;

namespace RTGen.Generation
{
    /// <summary>Generates a C++ interface or SmartPtr.</summary>
    public abstract class TemplateGenerator : BaseGenerator
    {
        /// <summary>Generate with the specified template and output to the <paramref name="outputPath"/>.</summary>
        /// <param name="templatePath">The path to the template file to use.</param>
        /// <param name="outputPath">The output path of the generated file.</param>
        // ReSharper disable once MemberCanBePrivate.Global
        protected void GenerateFile(string templatePath, string outputPath)
        {
            if (Log.Verbose)
            {
                Log.Info($"Generating {Options.Language} bindings for file: {RtFile.SourceFileName}");
            }

            GenerateOutput(RtFile.CurrentClass, templatePath, File.Open(outputPath, FileMode.Create));
        }

        /// <summary>Generate the output to a file.</summary>
        /// <param name="templatePath">The template to generate from. Pass <c>null</c> if not generating from a template.</param>
        public override void GenerateFile(string templatePath)
        {
            string tempPath = Path.GetTempFileName();
            GenerateFile(templatePath, tempPath);

            string outputFilePath = GetOutputFilePath();
            try
            {
                if (Log.Verbose)
                {
                    Log.Info($"  copying \"{tempPath}\" to \"{Path.GetFullPath(outputFilePath)}\"");
                }
                File.Copy(tempPath, outputFilePath, true);
                File.Delete(tempPath);
            }
            catch (Exception e)
            {
                throw new GeneratorException($"Failed to copy the generated file to the output directory.{(Log.Verbose ? $"({e.Message})" : null)}");
            }
        }

        /// <summary>Get the full path where to copy the generated file.</summary>
        /// <returns>Returns the full path to the file location where to copy the generated file.</returns>
        protected string GetOutputFilePath()
        {
            string fileName = Options.Filename
                              ?? Path.GetFileNameWithoutExtension(RtFile.SourceFileName)
                              ?? RtFile.SourceFileName;

            if (!string.IsNullOrEmpty(Options.GeneratedExtension) && Options.GeneratedExtension[0] != '.')
            {
                Options.GeneratedExtension = '.' + Options.GeneratedExtension;
            }

            Options.Filename = fileName;
            string newFileName = fileName + Options.FileNameSuffix + Options.GeneratedExtension;

            string outputPath = Path.Combine(Options.OutputDir, newFileName);
            return outputPath;
        }

        /// <summary>Generate the output to a string.</summary>
        /// <param name="templatePath">The template to generate from. Pass <c>null</c> if not generating from a template.</param>
        /// <returns>Returns the output source file as string.</returns>
        public override string Generate(string templatePath)
        {
            using (MemoryStream ms = new MemoryStream())
            {
                GenerateOutput(RtFile.CurrentClass, templatePath, ms);

                using (StreamReader sr = new StreamReader(ms))
                {
                    return sr.ReadToEnd();
                }
            }
        }

        /// <summary>Gives the generator an option to specify custom type names for RT CoreType types.</summary>
        /// <param name="mappings">The dictionary with the type mappings (CoreType => Generator type).</param>
        public override void RegisterTypeMappings(Dictionary<string, string> mappings)
        {

        }

        /// <summary>Gets the base include files/units/usings.</summary>
        /// <param name="rtFile"></param>
        /// <returns>Returns base include files/units/usings or <c>null</c> if none.</returns>
        protected virtual string GetIncludes(IRTFile rtFile)
        {
            return null;
        }

        private void GenerateOutput(IRTInterface rtClass, string templatePath, Stream outputStream)
        {
            templatePath = SetVariables(rtClass, templatePath);
            Variables.Add("headers", GetIncludes(RtFile));

            StreamReader template = null;
            try
            {
                FileInfo info = new FileInfo(templatePath);
                template = new StreamReader(info.Open(FileMode.Open, FileAccess.Read, FileShare.Read));

                using (StreamWriter output = new StreamWriter(outputStream))
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

                            return GetGlobalVariable(rtClass, templatePath, variable);
                        });

                        output.WriteLine(outputLine);
                    }
                }

                template.Close();
            }
            catch(Exception)
            {
                template?.Dispose();
                throw;
            }

            Variables.Clear();
        }

        /// <summary>Get the value of the specified variable in the currently generated interface.</summary>
        /// <param name="rtClass">The RT interface that is being generated.</param>
        /// <param name="variable">The variable to replace.</param>
        /// <returns>Returns <c>null</c> if the is unknown or unhandled otherwise the replaced variable or empty string.</returns>
        /// <remarks>If the returned value is NOT <c>null</c> it effectively overrides the base variables.</remarks>
        /// <remarks>If the returned value is <c>null</c> it checks for a base variable if exists otherwise reports a warning.</remarks>
        protected override string GetFileVariable(IRTInterface rtClass, string variable)
        {
            switch (variable)
            {
                case "BasePtrInterfaceTemplate":
                    string baseGenericTypes = null;
                    if (rtClass.BaseType.Flags.IsGeneric && rtClass.BaseType.GenericArguments != null)
                    {
                        baseGenericTypes = string.Join(", ", rtClass.BaseType.GenericArguments.Select(type => type.Name)) + ", ";
                    }

                    return rtClass.Type.Flags.IsGeneric
                               ? $"<{baseGenericTypes}{GetFileVariable(rtClass, "PtrIntferfaceArg")}>"
                               : $"<{baseGenericTypes}{rtClass.Type.Name}>";
                case "InterfaceTemplateDecl":
                    return !rtClass.Type.HasGenericArguments
                               ? rtClass.Type.Name
                               : $"{rtClass.Type.Name}<{string.Join($"{RtFile.AttributeInfo.ParameterSeparator} ", rtClass.Type.GenericArguments.Select(arg => arg?.Name))}>";
                case "InterfaceTemplate":
                    return !rtClass.Type.HasGenericArguments
                               ? rtClass.Type.Name
                               : $"{rtClass.Type.Name}<{string.Join($", ", rtClass.Type.GenericArguments.Select(arg => arg?.Name))}>";
                case "BaseInterfaceTemplate":
                    return rtClass.BaseType.GenericArguments?.Count != 0
                               ? rtClass.BaseType.GenericName()
                               : "";
            }

            return base.GetFileVariable(rtClass, variable);
        }

        /// <summary>Get global variable.</summary>
        /// <param name="rtClass"></param>
        /// <param name="templatePath"></param>
        /// <param name="variable"></param>
        /// <returns></returns>
        protected string GetGlobalVariable(IRTInterface rtClass, string templatePath, string variable)
        {
            string customVariable = GetFileVariable(rtClass, variable);
            if (customVariable != null)
            {
                return customVariable;
            }

            if (Variables.TryGetValue(variable, out string globalVariable))
            {
                return globalVariable;
            }

            LogIgnoredVariable(variable, templatePath);
            return string.Empty;
        }

        /// <summary>Writes the RTInterface method wrappers to the string buffer.</summary>
        /// <param name="rtClass">The interface for which to generate the wrappers.</param>
        /// <param name="templatePath">The path to the template from which to generate.</param>
        /// <returns>Returns a string buffer with the generated method wrappers.</returns>
        protected override StringBuilder WriteMethodWrappers(IRTInterface rtClass, string templatePath)
        {
            StringBuilder methods = new StringBuilder();

            string fileName = Path.GetFileNameWithoutExtension(templatePath) + ".impl.method.template";
            string fileNameRet = Path.GetFileNameWithoutExtension(templatePath) + ".impl.ret.method.template";

            string methodTemplatePath = Utility.GetTemplate(fileName);
            string methodTemplatePathRet = Utility.GetTemplate(fileNameRet);

            string methodImplTemplate = File.ReadAllText(methodTemplatePath);
            string methodImplRetTemplate = File.ReadAllText(methodTemplatePathRet);

            foreach (IMethod method in rtClass.Methods)
            {
                if (method.IsIgnored.HasFlag(GeneratorType.Wrapper))
                {
                    continue;
                }

                string methodImplRetSelfTemplate = "";
                try
                {
                    string fileNameRetSelf = Path.GetFileNameWithoutExtension(templatePath) + ".impl.ret.self.method.template";
                    string methodTemplatePathRetSelf = Utility.GetTemplate(fileNameRetSelf);
                    methodImplRetSelfTemplate = File.ReadAllText(methodTemplatePathRetSelf);
                }
                catch(TemplateNotFoundException)
                {
                }

                if (method.ReturnSelf && methodImplRetTemplate != "")
                {
                    WriteMethodWrapper(method, methodImplRetSelfTemplate, methods);
                }
                else
                {
                    bool returnsByRef = method.ReturnsByRef();

                    WriteMethodWrapper(method, returnsByRef
                                                ? methodImplRetTemplate
                                                : methodImplTemplate, methods);
                }

                methods.AppendLine();
            }

            return methods;
        }

        /// <summary>Get the value of the specified variable for the currently generated wrapper method.</summary>
        /// <param name="method">The method for which the wrapper is being generated.</param>
        /// <param name="variable">The variable to replace.</param>
        /// <returns>Returns <c>null</c> if the is unknown or unhandled otherwise the replaced variable or empty string.</returns>
        /// <remarks>If the returned value is not null it effectively overrides the base variables.</remarks>
        /// <remarks>If the returned value is <c>null</c> it checks for a base variable if exists otherwise reports a warning.</remarks>
        protected virtual string GetMethodWrapperVariable(IOverload method, string variable)
        {
            return null;
        }

        /// <summary>Generates a method wrapper for <paramref name="method"/> using the specified <paramref name="template"/>.</summary>
        /// <param name="method">The method for which to generate a wrapper.</param>
        /// <param name="template">The template string to use for generating.</param>
        /// <param name="methods">Buffer where to write the method wrapper.</param>
        protected virtual void WriteMethodWrapper(IMethod method, string template, StringBuilder methods)
        {
            foreach (IOverload overload in method.Overloads)
            {
                string generatedMethod = ReplacementRegex.Replace(template, m =>
                {
                    string variable = m.Groups[1].Value;

                    string customVariable = GetMethodWrapperVariable(overload, variable);
                    if (customVariable != null)
                    {
                        return customVariable;
                    }

                    return GetMethodWrapperVariableInternal(overload, variable);
                });

                methods.AppendLine(generatedMethod);
                if (method.Documentation != null)
                {
                    methods.AppendLine();
                }
            }

            if (method.Overloads.Count > 1)
            {
                methods.AppendLine();
            }
        }

        /// <summary>Gets the common variable values for the specified method.</summary>
        /// <param name="overload">The method for which to get the variable value.</param>
        /// <param name="variable">The variable name for which to get the value.</param>
        /// <returns>Returns the value of the variable for the specified method.</returns>
        protected string GetMethodWrapperVariableInternal(IOverload overload, string variable)
        {
            IArgument arg;

            IMethod method = overload.Method;

            switch (variable)
            {
                case "Name":
                    return method.Name;
                case "ReturnType":
                    if (overload.ReturnType != null)
                    {
                        return overload.ReturnType.FullName();
                    }

                    return string.Empty;
                case "ReturnTypePtr":
                {
                    arg = overload.GetLastByRefArgument();
                    if (arg != null)
                    {
                        if (!RtFile.TypeAliases.TryGetValue(arg.Type.UnmappedName, out ITypeName type))
                        {
                            type = arg.Type;
                        }

                        string argTypeFull = type.ReturnType();
                        return arg.IsConst
                                   ? "const " + argTypeFull
                                   : argTypeFull;
                    }

                    return string.Empty;
                }
                case "ReturnTypeName":
                    if (overload.ReturnType != null)
                    {
                        return overload.ReturnType.Name;
                    }

                    return string.Empty;
                case "ExitReturnArgName":
                case "ReturnArgName":
                    arg = overload.GetLastByRefArgument();
                    if (arg != null)
                    {
                        return GetArgumentName(overload, arg);
                    }

                    return string.Empty;
                case "ReturnArgTypeName":
                    arg = overload.GetLastByRefArgument();
                    if (arg != null)
                    {
                        return arg.Type.Name;
                    }

                    return string.Empty;
                case "CallingConvention":
                    if (!string.IsNullOrEmpty(method.CallingConvention))
                    {
                        return method.CallingConvention;
                    }

                    return string.Empty;
                case "CallingConventionMacro":
                    if (!string.IsNullOrEmpty(method.CallingConvention))
                    {
                        return method.GetCallingConventionMacro();
                    }

                    return string.Empty;
                case "PtrNameOnly":
                    if (Variables.TryGetValue("PtrNameOnly", out string ptrName))
                    {
                        return ptrName;
                    }

                    return string.Empty;
                default:
                    LogIgnoredVariable(variable, "Unknown");
                    return string.Empty;
            }
        }
    }
}
