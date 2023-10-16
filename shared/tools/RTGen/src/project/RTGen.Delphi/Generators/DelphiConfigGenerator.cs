using System;
using System.IO;
using RTGen.Exceptions;
using RTGen.Generation;
using RTGen.Interfaces;
using RTGen.Types;
using RTGen.Util;

namespace RTGen.Delphi.Generators
{
    // ReSharper disable once ClassNeverInstantiated.Global
    public class DelphiConfigGenerator : TemplateGenerator, IConfigGenerator
    {
        public override IVersionInfo Version => new VersionInfo
        {
            Major = 3,
            Minor = 0,
            Patch = 0
        };

        /// <summary>Generate the config to a file.</summary>
        /// <param name="templatePath">The template to generate from. Pass <c>null</c> if not generating from a template.</param>
        public void GenerateConfigFile(string templatePath)
        {
            Initialize();

            string tempPath = Path.GetTempFileName();
            GenerateConfigOutput(templatePath, File.Open(tempPath, FileMode.Create));

            string outputFilePath = GetOutputFilePath();
            try
            {
                File.Copy(tempPath, outputFilePath, true);
                File.Delete(tempPath);
            }
            catch (Exception e)
            {
                throw new GeneratorException(
                    $"Failed to copy the generated config file to the output directory.{(Log.Verbose ? $"({e.Message})" : null)}");
            }
        }

        private void GenerateConfigOutput(string templatePath, Stream outputStream)
        {
            templatePath = SetVariables(null, templatePath);
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

                            return GetConfigVariable(Options.LibraryInfo, templatePath, variable);
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

            Variables.Clear();
        }

        private string GetConfigVariable(ILibraryInfo libInfo, string templatePath, string variable)
        {
            string result = GetVariable(libInfo, variable);
            if (result != null)
            {
                return result;
            }

            if (Variables.ContainsKey(variable))
            {
                return Variables[variable];
            }

            LogIgnoredVariable(variable, templatePath);
            return string.Empty;
        }

        private string GetVariable(ILibraryInfo libInfo, string variable)
        {
            switch (variable)
            {
                case "LibraryName":
                    return libInfo.Name;
                case "CapitalizedLibraryName":
                    return libInfo.Name.Capitalize();
                case "LibraryOutput":
                    return libInfo.OutputName;
                case "Namespace":
                    return libInfo.Namespace?.ToString(".");
                case "LibraryMajorVersion":
                    return libInfo.Version.Major.ToString();
                default:
                    return null;
            }
        }

        private void Initialize()
        {
            string ns = Options.LibraryInfo.Namespace?.ToString(".");
            string libName = Options.LibraryInfo.Name;

            Options.Filename = !string.IsNullOrEmpty(ns)
                                   ? ns + "." + libName
                                   : libName;

            Options.Filename += ".Config";
            if (string.IsNullOrEmpty(Options.GeneratedExtension))
            {
                Options.GeneratedExtension = ".pas";
            }
        }
    }
}
