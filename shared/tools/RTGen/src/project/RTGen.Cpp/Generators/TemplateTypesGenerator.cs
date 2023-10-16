using System;
using System.IO;
using RTGen.Util;
using RTGen.Types;
using RTGen.Interfaces;
using System.Collections.Generic;

namespace RTGen.Cpp.Generators
{
    // ReSharper disable once ClassNeverInstantiated.Global
    public class TemplateTypesGenerator : IGenerator
    {
        public IGeneratorOptions Options { get; set; }

        public IVersionInfo Version => new VersionInfo
        {
            Major = 2,
            Minor = 0,
            Patch = 0
        };

        public IRTFile RtFile { get; set; }

        public void GenerateFile(string templatePath)
        {
            string json = Json.Serialize(RtFile.Tag);

            string fullPath = Path.Combine(Options.OutputDir, $"{Path.GetFileNameWithoutExtension(RtFile.SourceFileName)}.json");
            File.WriteAllText(fullPath, json);
        }

        public string Generate(string templatePath)
        {
            throw new NotImplementedException();
        }

        public void RegisterTypeMappings(Dictionary<string, string> mappings)
        {
        }
    }
}
