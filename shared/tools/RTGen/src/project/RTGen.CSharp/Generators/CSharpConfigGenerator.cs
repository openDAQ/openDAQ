using System;
using System.IO;
using System.Linq;

using RTGen.Exceptions;
using RTGen.Generation;
using RTGen.Interfaces;
using RTGen.Types;
using RTGen.Util;


namespace RTGen.CSharp.Generators
{
    // ReSharper disable once ClassNeverInstantiated.Global
    public class CSharpConfigGenerator : TemplateGenerator, IConfigGenerator
    {
        private string _outputDir;
        private string _licenseFilePath;

        public override IVersionInfo Version => new VersionInfo
        {
            Major = 1,
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

            //get target names from options (unify path separators using Path.GetFullPath)
            string outputFilePath = Path.GetFullPath(GetOutputFilePath());
            string orgOutputDir = Path.GetFullPath(base.Options.OutputDir);

            outputFilePath = outputFilePath.Replace(orgOutputDir, _outputDir); //re-target output directory
            try
            {
                if (Log.Verbose)
                {
                    Log.Info($"  copying \"{tempPath}\" to \"{outputFilePath}\"");
                }
                File.Copy(tempPath, outputFilePath, true);
                File.Delete(tempPath);
            }
            catch (Exception e)
            {
                throw new GeneratorException(
                    $"Failed to copy the generated config file to the output directory.{(Log.Verbose ? $"({e.Message})" : null)}");
            }
        }

        private void Initialize()
        {
            string[] namespaceParts = base.Options.LibraryInfo.Namespace?.Components ?? System.Array.Empty<string>(); //(base.Options.LibraryInfo.Namespace?.Raw ?? "daq").Split('.');

            //adapt library and file name
            base.Options.LibraryInfo.Name = base.Options.LibraryInfo.Name.Capitalize();
            base.Options.Filename = $"{base.Options.LibraryInfo.Name}DllInfo";

            //adapt output directory
            //assuming we are in a namespace hierarchy we want to put this file into its parent folder
            _outputDir = Path.GetFullPath(base.Options.OutputDir); //unify path separators using Path.GetFullPath
            string subFolderName = Path.DirectorySeparatorChar + namespaceParts.Last();
            while (_outputDir.EndsWith(subFolderName, StringComparison.OrdinalIgnoreCase))
                _outputDir = _outputDir.Substring(0, _outputDir.Length - subFolderName.Length);

            //adapt namespace
            if (base.Options.LibraryInfo.Namespace != null)
            {
                //put this class into its parent namespace (if in sub-namespace) since it is generated from names only
                string lastNamespacePart = "." + namespaceParts.Last();
                string @namespace = string.Join(".", namespaceParts.Select(str => str.Capitalize())); //capitalize all parts
                while (@namespace.EndsWith(lastNamespacePart, StringComparison.OrdinalIgnoreCase))
                    @namespace = @namespace.Substring(0, @namespace.Length - lastNamespacePart.Length);
                base.Options.LibraryInfo.Namespace.Raw = @namespace;
            }

            //adapt extension
            if (string.IsNullOrEmpty(base.Options.GeneratedExtension))
            {
                base.Options.GeneratedExtension = ".cs";
            }
        }

        private void GenerateConfigOutput(string templatePath, Stream outputStream)
        {
            templatePath = SetVariables(null, templatePath);

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

                            return GetConfigVariable(base.Options.LibraryInfo, templatePath, variable);
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

        private string GetConfigVariable(ILibraryInfo libraryInfo, string templatePath, string variable)
        {
            string result = GetVariable(libraryInfo, variable);
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
                case "LibraryOutput":
                    return libInfo.OutputName;
                case "LibraryMajorVersion":
                    return libInfo.Version.Major.ToString();
                case "LibraryMinorVersion":
                    return libInfo.Version.Minor.ToString();
                case "LibraryPatchVersion":
                    return libInfo.Version.Patch.ToString();
                case "Namespace":
                    return libInfo.Namespace?.ToString(this.RtFile?.AttributeInfo?.NamespaceSeparator ?? ".") ?? "Daq";
                case "CSLicenseComment":
                    return GetLicenseHeader();
                default:
                    return null;
            }


            //=== local functions =================================================================

            string GetLicenseHeader()
            {
                if (string.IsNullOrEmpty(_licenseFilePath))
                {
                    string licenseFile = FindFile(base.Options.OutputDir, "LICENSE.txt");

                    if (!File.Exists(licenseFile))
                        licenseFile = FindFile(base.Options.OutputDir, "license.in");

                    if (!File.Exists(licenseFile))
                        licenseFile = FindFile(base.Options.OutputDir, @"shared\tools\license-header-checker\license.in");

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
    }
}
