using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.CompilerServices;
using System.Text;
using System.Text.RegularExpressions;
using RTGen.Interfaces;
using RTGen.Util;

namespace RTGen.Generation
{
    /// <summary>Abstract base for all template generators.</summary>
    public abstract class BaseGenerator : IGenerator
    {
        /// <summary>Regular expression for substituting template variables with generator text.</summary>
        protected static readonly Regex ReplacementRegex =
            new Regex(@"\$([\S]*?)\$", RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.Singleline);

        /// <summary>A map of variable names to text they will be replaced with.</summary>
        protected Dictionary<string, string> Variables;

        /// <summary>The number of spaces with which to prefix the method declaration.</summary>
        protected int IndentationSpaces
        {
            set => _indentation = new string(' ', value);
            get => _indentation.Length;
        }

        /// <summary>The method declaration indentation.</summary>
        protected string Indentation => _indentation;

        private string _indentation = "    ";
        private IGeneratorOptions _options;

        /// <summary>Constructs Base generator without any options defined.</summary>
        protected BaseGenerator()
        {
            GenerateMethods = true;
        }

        /// <summary>Constructs Base generator with specified options.</summary>
        protected BaseGenerator(IGeneratorOptions options)
        {
            _options = options;
        }

        /// <summary>Generator version info.</summary>
        /// <returns>Returns the generator version info.</returns>
        public abstract IVersionInfo Version { get; }

        /// <summary>Object description of the source file.</summary>
        public virtual IRTFile RtFile { get; set; }

        /// <summary>Additional generation options (file name, suffix, output dir ...).</summary>
        public virtual IGeneratorOptions Options
        {
            get => _options;
            set => _options = value;
        }

        public bool GenerateMethods { get; set; }

        /// <summary>Generate the output to a file.</summary>
        /// <param name="templatePath">The template to generate from. Pass <c>null</c> if not generating from a template.</param>
        public abstract void GenerateFile(string templatePath);

        /// <summary>Generate the output to a string.</summary>
        /// <param name="templatePath">The template to generate from. Pass <c>null</c> if not generating from a template.</param>
        /// <returns>Returns the output source file as string.</returns>
        public abstract string Generate(string templatePath);

        /// <summary>Gives the generator an option to specify custom type names for RT CoreType types.</summary>
        /// <param name="mappings">The dictionary with the type mappings (CoreType => Generator type).</param>
        public abstract void RegisterTypeMappings(Dictionary<string, string> mappings);

        /// <summary>Generates methods in the <paramref name="rtClass"/> interface using the <paramref name="baseTemplatePath"/> template.</summary>
        /// <param name="rtClass">The RT interface info.</param>
        /// <param name="baseTemplatePath">The base template file path.</param>
        protected virtual StringBuilder WriteMethods(IRTInterface rtClass, string baseTemplatePath)
        {
            StringBuilder methods = new StringBuilder();

            string fileName = Utility.GetTemplate(Path.GetFileNameWithoutExtension(baseTemplatePath) + ".method.template");

            string methodTemplatePath = Path.Combine(
                Path.GetDirectoryName(baseTemplatePath) ?? "",
                fileName
            );

            if (!File.Exists(methodTemplatePath))
            {
                Log.Warning("Method template does not exist. Expected file: " + fileName);
                return null;
            }

            string[] methodTemplate = File.ReadAllLines(methodTemplatePath);

            string procTemplate = methodTemplate[0];
            string funcTemplate = methodTemplate[1];
            string outArgTemplate = methodTemplate[2];
            string inArgTemplate = methodTemplate[3];
            string separator = methodTemplate[4];

            foreach (IMethod method in rtClass.Methods)
            {
                string template = method.ReturnType == null
                                      ? procTemplate
                                      : funcTemplate;

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

                    switch (variable)
                    {
                        case "Name":
                            return method.Name;
                        case "ReturnType":
                            if (method.ReturnType != null)
                            {
                                return method.ReturnType.FullName();
                            }

                            return string.Empty;
                        case "ReturnTypeName":
                            if (method.ReturnType != null)
                            {
                                return method.ReturnType.Name;
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
                        case "Arguments":
                            return GetMethodArguments(method.Overloads[0], outArgTemplate, inArgTemplate, separator);
                        default:
                            LogIgnoredVariable(variable, fileName);
                            return string.Empty;
                    }
                });

                methods.AppendLine(_indentation + generatedMethod);
            }

            return methods;
        }

        /// <summary>Checks if member method should use the default generation.</summary>
        /// <param name="rtClass">The instance for which to generate the member.</param>
        /// <param name="method">The method member info to generate.</param>
        /// <returns>Returns <c>false</c> if the member should not be generated otherwise <c>true</c>.</returns>
        protected virtual bool HandleMemberMethod(IRTInterface rtClass, IMethod method)
        {
            return true;
        }

        /// <summary>Logs a warning for the unknown variable in specified section / template.</summary>
        /// <param name="variable">The unknown variable.</param>
        /// <param name="templateName">The section or template where the variable was encountered.</param>
        /// <param name="memberName"></param>
        /// <param name="sourceFilePath"></param>
        /// <param name="sourceLineNumber"></param>
        protected void LogIgnoredVariable(string variable, string templateName,
                                          [CallerMemberName] string memberName = "",
                                          [CallerFilePath] string sourceFilePath = "",
                                          [CallerLineNumber] int sourceLineNumber = 0)
        {
            Log.Warning(Log.Verbose
                            ? $"Ignoring unknown variable ${variable}$ in \"{Path.GetFileName(templateName)}\" [{memberName}() at {Path.GetFileName(sourceFilePath)}:{sourceLineNumber}]."
                            : $"Ignoring unknown variable ${variable}$ in \"{Path.GetFileName(templateName)}\"."
            );
        }

        /// <summary>Get the value of the specified variable for the currently generated method.</summary>
        /// <param name="method">The method that is being generated.</param>
        /// <param name="variable">The variable to replace.</param>
        /// <returns>Returns <c>null</c> if the is unknown or unhandled otherwise the replaced variable or empty string.</returns>
        /// <remarks>If the returned value is not null it effectively overrides the base variables.</remarks>
        /// <remarks>If the returned value is <c>null</c> it checks for a base variable if exists otherwise reports a warning.</remarks>
        protected virtual string GetMethodVariable(IMethod method, string variable)
        {
            return null;
        }

        /// <summary>Get the value of the specified variable for the currently generated method argument.</summary>
        /// <param name="arg">The method argument that is being generated.</param>
        /// <param name="overload">The method overload that is being generated.</param>
        /// <param name="variable">The variable to replace.</param>
        /// <returns>Returns <c>null</c> if the is unknown or unhandled otherwise the replaced variable or empty string.</returns>
        /// <remarks>If the returned value is not null it effectively overrides the base variables.</remarks>
        /// <remarks>If the returned value is <c>null</c> it checks for a base variable if exists otherwise reports a warning.</remarks>
        protected virtual string GetMethodArgumentVariable(IArgument arg, IOverload overload, string variable)
        {
            return null;
        }

        /// <summary>Generates method arguments to a string using the specified templates and a separator.</summary>
        /// <param name="overload">The method overload that contains the parameters.</param>
        /// <param name="outParamTemplate">The template for out/by-ref parameters.</param>
        /// <param name="inParamTemplate">The template for normal in-only parameters.</param>
        /// <param name="separator">The separator between arguments.</param>
        /// <returns>Returns the generated method's arguments as a string.</returns>
        public virtual string GetMethodArguments(IOverload overload, string outParamTemplate, string inParamTemplate, string separator)
        {
            StringBuilder sb = new StringBuilder();

            IArgument arg = overload.GetLastByRefArgument();

            foreach (IArgument argument in overload.Arguments)
            {
                if (Options.GenerateWrapper && argument == arg)
                {
                    continue;
                }

                string template = argument.IsOutParam
                                      ? outParamTemplate
                                      : inParamTemplate;

                sb.Append(ReplacementRegex.Replace(template, m =>
                {
                    string variable = m.Groups[1].Value;

                    string customVariable = GetMethodArgumentVariable(argument, overload, variable);
                    if (customVariable != null)
                    {
                        return customVariable;
                    }

                    return GetMethodArgumentVariableInternal(argument, overload, variable);
                }));
                sb.Append(separator);
            }

            if (sb.Length > separator.Length)
            {
                sb.Remove(sb.Length - separator.Length, separator.Length);
            }

            return sb.ToString();
        }

        /// <summary></summary>
        /// <param name="argument">The method argument that is being generated.</param>
        /// <param name="overload">The method overload that contains the parameters.</param>
        /// <param name="variable">The variable to replace.</param>
        /// <returns></returns>
        protected string GetMethodArgumentVariableInternal(IArgument argument, IOverload overload, string variable)
        {
            switch (variable)
            {
                case "ArgName":
                    return GetArgumentName(overload, argument);
                case "ArgTypeName":
                    if (argument.Type != null)
                    {
                        return argument.Type.Name;
                    }

                    return string.Empty;
                case "ArgTypeFull":
                    if (argument.Type != null)
                    {
                        return argument.Type.FullName();
                    }

                    return string.Empty;
                case "ArgTypeModifiers":
                    if (argument.Type != null)
                    {
                        return argument.Type.Modifiers;
                    }

                    return string.Empty;
                default:
                    return string.Empty;
            }
        }

        /// <summary>Helper to get method wrapper argument names to pass to the actual interface method.</summary>
        /// <param name="overload">The method overload being generated.</param>
        /// <param name="separator">Argument separators (e.g. C++ ',' or Delphi ';')</param>
        /// <param name="refSymbol">Address of operator (e.g. C++ '&amp;', or Delphi '@')</param>
        /// <returns>Returns the argument names to pass to the actual interface method.</returns>
        /// <example>
        /// <code>ErrCode getItemAt(Int index, IString** text)</code>
        /// <remarks>Wrapper =></remarks>
        /// <code>
        ///           StringPtr getText(Int index) {
        ///             StringPtr text;
        ///             getText(index, &amp;text);
        ///           }
        /// </code>
        /// </example>
        public virtual string GetArgumentNames(IOverload overload, string separator, string refSymbol)
        {
            StringBuilder names = new StringBuilder();
            foreach (IArgument argument in overload.Arguments)
            {
                if (argument.IsOutParam)
                {
                    names.Append(refSymbol);
                }

                names.Append(GetArgumentName(overload, argument));
                names.Append(separator);
            }

            if (names.Length > separator.Length)
            {
                names.Remove(names.Length - separator.Length, separator.Length);
            }

            return names.ToString();
        }

        /// <summary>Computes the valid argument in the generated language.</summary>
        /// <param name="overload">The method overload in which the argument is used.</param>
        /// <param name="argument">The argument as parsed.</param>
        /// <returns>Returns the valid argument in the generated language.</returns>
        protected virtual string GetArgumentName(IOverload overload, IArgument argument)
        {
            return argument.Name;
        }

        /// <summary>Get the value of the specified variable in the currently generated interface.</summary>
        /// <param name="rtClass">The RT interface that is being generated.</param>
        /// <param name="variable">The variable to replace.</param>
        /// <returns>Returns <c>null</c> if the is unknown or unhandled otherwise the replaced variable or empty string.</returns>
        /// <remarks>If the returned value is NOT <c>null</c> it effectively overrides the base variables.</remarks>
        /// <remarks>If the returned value is <c>null</c> it checks for a base variable if exists otherwise reports a warning.</remarks>
        protected virtual string GetFileVariable(IRTInterface rtClass, string variable)
        {
            return null;
        }

        /// <summary>Render template to a string. Assumes the template is UTF8 encoded.</summary>
        /// <param name="model">The model to pass to the callback for variable substitution.</param>
        /// <param name="template">The template to parse for variables.</param>
        /// <param name="variableCallback">The callback to call when finding a variable.</param>
        /// <returns>Rendered template.</returns>
        protected string RenderTemplate<T>(T model, string template, Func<T, string, string> variableCallback)
        {
            using (MemoryStream stringTemplate = new MemoryStream(Encoding.UTF8.GetBytes(template)))
            {
                return RenderTemplate(model, stringTemplate, variableCallback);
            }
        }

        /// <summary>Render template to a string. Assumes the template is UTF8 encoded.</summary>
        /// <param name="model">The model to pass to the callback for variable substitution.</param>
        /// <param name="templateFile">The path to the template to parse for variables.</param>
        /// <param name="variableCallback">The callback to call when finding a variable.</param>
        /// <returns>Rendered template.</returns>
        protected string RenderFileTemplate<T>(T model, string templateFile, Func<T, string, string> variableCallback)
        {
            FileInfo template = new FileInfo(templateFile);

            using (FileStream stringTemplate = template.Open(FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                return RenderTemplate(model, stringTemplate, variableCallback);
            }
        }

        /// <summary>Render template to a string.</summary>
        /// <param name="model"></param>
        /// <param name="templateStream"></param>
        /// <param name="variableCallback"></param>
        /// <returns>Rendered template.</returns>
        protected string RenderTemplate<T>(T model, Stream templateStream, Func<T, string, string> variableCallback)
        {
            StreamReader template = new StreamReader(templateStream);
            StringBuilder output = new StringBuilder();

            while (!template.EndOfStream)
            {
                string templateLine = template.ReadLine();

                if (string.IsNullOrEmpty(templateLine))
                {
                    output.AppendLine(templateLine);
                    continue;
                }

                string outputLine = ReplacementRegex.Replace(templateLine, m =>
                {
                    string variable = m.Groups[1].Value;

                    string customVariable = variableCallback(model, variable);
                    if (customVariable != null)
                    {
                        return customVariable;
                    }

                    if (Variables.TryGetValue(variable, out string variableValue))
                    {
                        return variableValue;
                    }

                    LogIgnoredVariable(variable, "RenderTemplate");
                    return string.Empty;
                });

                output.AppendLine(outputLine);
            }

            return output.ToString();
        }

        /// <summary></summary>
        /// <param name="rtClass"></param>
        /// <param name="templatePath"></param>
        /// <returns></returns>
        protected abstract StringBuilder WriteMethodWrappers(IRTInterface rtClass, string templatePath);

        /// <summary>Set base variables for the specified interface.</summary>
        /// <param name="rtClass">The interface from which to gather the data.</param>
        /// <param name="templatePath">The base template path from which to generate.</param>
        /// <returns>Returns the path to the template to use for main generation process.</returns>
        protected string SetVariables(IRTInterface rtClass, string templatePath)
        {
            IVersionInfo version = this.Version;

            Variables = new Dictionary<string, string>
            {
                { "CustomTypes", "" },
                { "DateAndTime", Options.UseDebugTimeStamp ? "D-E-B-U-G" : DateTime.Now.ToString("dd.MM.yyyy HH:mm:ss") },
                { "GeneratorVersion", $"v{version.Major}.{version.Minor}.{version.Patch}" },
                { "GeneratorName", this.GetType().Name }
            };

            if (RtFile != null)
            {
                ITypeName interfaceType = rtClass.Type;
                ITypeName baseType = rtClass.BaseType;

                Variables.Add("Interface", interfaceType.Name);
                Variables.Add("InterfaceFull", interfaceType.FullName());
                Variables.Add("NonInterfaceType", interfaceType.NonInterfaceName);
                Variables.Add("BaseType", baseType.Name);
                Variables.Add("BaseTypeFull", baseType.FullName());
                Variables.Add("BaseTypeNonInterface", baseType.NonInterfaceName);
                Variables.Add("PtrName", interfaceType.Wrapper.Name);
                Variables.Add("PtrNameOnly", interfaceType.Wrapper.NameOnly);
                Variables.Add("PtrNameFull", interfaceType.Wrapper.NameFull);
                Variables.Add("PtrNameQualified", interfaceType.Wrapper.NameQualified);
                Variables.Add("BasePtrName", baseType.Wrapper.BaseName);
                Variables.Add("BasePtrNameOnly", baseType.Wrapper.NameOnly);
                Variables.Add("BasePtrNameFull", baseType.Wrapper.BaseNameFull);
                Variables.Add("start_namespace", RtFile?.StartNamespaceMacro);
                Variables.Add("end_namespace", RtFile?.EndNamespaceMacro);
                Variables.Add("InterfaceGuid", interfaceType.Guid.ToString("D"));
                Variables.Add("InterfaceGuidX", interfaceType.Guid.ToString("X"));
                Variables.Add("SourceFileName", Path.GetFileNameWithoutExtension(RtFile?.SourceFileName));
                Variables.Add("ControlTagName", interfaceType.ControlTagName);
            }

            SetCustomVariables();

            if (RtFile != null && GenerateMethods)
            {
                StringBuilder methods;
                if (Options.GenerateWrapper)
                {
                    methods = WriteMethodWrappers(rtClass, templatePath);

                    templatePath = (Path.GetFileNameWithoutExtension(templatePath) ?? "") + ".ptr.template";
                    templatePath = Utility.GetTemplate(templatePath);
                }
                else
                {
                    templatePath = Utility.GetTemplate(templatePath);
                    methods = WriteMethods(rtClass, templatePath);
                }

                methods.TrimTrailingNewLines();
                Variables.Add("Methods", methods.ToString());
            }

            OnVariablesReady();

            return templatePath;
        }

        /// <summary>Called when all variables have ben set before main output generation.</summary>
        protected virtual void OnVariablesReady()
        {
        }

        /// <summary>Called after every-time the variables are reset, before the start of the generation process.</summary>
        protected virtual void SetCustomVariables()
        {
        }
    }
}
