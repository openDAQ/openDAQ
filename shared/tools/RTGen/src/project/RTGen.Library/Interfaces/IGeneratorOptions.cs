using System;

namespace RTGen.Interfaces
{
    /// <summary>Additional generator options.</summary>
    public interface IGeneratorOptions : ICloneable
    {
        /// <summary>The language the file object description was parsed from.</summary>
        string InputLanguage { get; }

        /// <summary>The desired language of the output file.</summary>
        string Language { get; }

        /// <summary>The base file name without an extension that will be generated.</summary>
        string Filename { get; set; }

        /// <summary>The suffix to append to the source file.</summary>
        /// <example><c>_ptr</c> some_class => some_class_ptr </example>
        string FileNameSuffix { get; set; }

        /// <summary>The extension of the generated file.</summary>
        /// <example><c>.h</c> for C++, <c>.pas</c> for Delphi etc.</example>
        string GeneratedExtension { get; set; }

        /// <summary>The desired output directory in which to place the generated file (headers).</summary>
        string OutputDir { get; }

        /// <summary>
        /// The desired output directory in which to place the generated file (source).
        /// Only used when generating languages with split declarations and definitions.
        /// </summary>
        string OutputSourceDir { get; }

        /// <summary>Generate Wrappers/SmartPointers (e.g. for C++).</summary>
        bool GenerateWrapper { get; set; }

        /// <summary>Whether instead of generating code, generate library load/config header.</summary>
        bool GenerateConfig { get; set; }

        /// <summary>Library version, name and platform info.</summary>
        ILibraryInfo LibraryInfo { get; set; }

        /// <summary>Provide fixed "Generated At:" time to the generators.</summary>
        bool UseDebugTimeStamp { get; }
    }
}
