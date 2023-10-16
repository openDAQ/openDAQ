using System.Collections.Generic;
using RTGen.Interfaces;
using RTGen.Types;

namespace RTGen.Util
{
    /// <summary>Represents program command line options.</summary>
    public class ProgramOptions : IGeneratorOptions, IParserOptions
    {
        /// <summary>Creates an empty instance of program options.</summary>
        public ProgramOptions()
        {
            LibraryInfo = new LibraryInfo();
            PredefinedTypeArguments = new Dictionary<string, ISampleTypes>();
        }

        /// <summary>The source file to parse.</summary>
        public string InputFile { get; set; }

        /// <summary>Generate Wrappers/SmartPointers (e.g. for C++).</summary>
        public bool GenerateWrapper { get; set; }

        /// <summary>Which template to pass to generator (override default lang.template)</summary>
        public string TemplatePath { get; set; }

        /// <summary>Show console options summary.</summary>
        public bool ShowHelp { get; set; }

        /// <summary>Whether to stop on first parse error.</summary>
        public bool ContinueOnParseErrors { get; set; }

        /// <summary>Whether to log additional input useful for troubleshooting.</summary>
        public bool Verbose { get; set; }

        /// <summary>The language the file object description was parsed from.</summary>
        public string InputLanguage { get; set; }

        /// <summary>The desired language of the output file.</summary>
        public string Language { get; set; }

        /// <summary>The base file name without an extension.</summary>
        public string Filename { get; set; }

        /// <summary>The suffix to append to the source file.</summary>
        /// <example><c>_ptr</c> some_class => some_class_ptr </example>
        public string FileNameSuffix { get; set; }

        /// <summary>The extension of the generated file.</summary>
        /// <example><c>.h</c> for C++, <c>.pas</c> for Delphi etc.</example>
        public string GeneratedExtension { get; set; }

        /// <summary>The desired output directory in which to place the generated file (headers).</summary>
        public string OutputDir { get; set; }

        /// <summary>
        /// The desired output directory in which to place the generated file (source).
        /// Only used when generating languages with split declarations and definitions.
        /// </summary>
        public string OutputSourceDir { get; set; }

        /// <summary>Output UUID v5 for the specified name only. (No parse/generate should occur.)</summary>
        public bool GenerateGuid { get; set; }

        /// <summary>Whether instead of generating code, generate library load/config header.</summary>
        public bool GenerateConfig { get; set; }

        /// <summary>Library version, name and platform info.</summary>
        public ILibraryInfo LibraryInfo { get; set; }

        /// <summary>The CoreTypes namespace override.</summary>
        public INamespace CoreNamespace { get; set; }

        /// <summary>Use RT Core namespace for generating the interface UUID.</summary>
        public bool RtGuid { get; set; }

        /// <summary>Print version information for all components.</summary>
        public bool PrintVersion { get; set; }

        /// <summary>Provide fixed "Generated At:" time to the generators.</summary>
        public bool UseDebugTimeStamp { get; set; }

        /// <summary>Predefined type arguments (as macros or constants).</summary>
        public IDictionary<string, ISampleTypes> PredefinedTypeArguments { get; set; }

        /// <summary>Shallow copies the object.</summary>
        /// <returns>A shallow copy of the object.</returns>
        public object Clone()
        {
            return this.MemberwiseClone();
        }
    }
}
