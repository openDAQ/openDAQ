using System;
using System.Collections.Generic;

namespace RTGen.Interfaces
{
    /// <summary>Represents the type of generators used by RTGen.</summary>
    [Flags]
    public enum GeneratorType
    {
        /// <summary>Unknown or none.</summary>
        None,
        /// <summary>Generates an interface only.</summary>
        Interface,
        /// <summary>Generates an easy to use wrapper around the interface instance.</summary>
        Wrapper,
        /// <summary>Generates a decorator around the interface instance.</summary>
        Decorator,
        /// <summary>Generates a property class and base property object instance class.</summary>
        PropertyClass
    }

    /// <summary>Generates an output file.</summary>
    public interface IGeneratorBase
    {
        /// <summary>Additional generation options (file name, suffix, output dir ...).</summary>
        IGeneratorOptions Options { get; set; }

        /// <summary>Generator version info.</summary>
        /// <returns>Returns the generator version info.</returns>
        IVersionInfo Version { get; }
    }

    /// <summary>Generates an output file, from the object description of the source file.</summary>
    public interface IGenerator : IGeneratorBase
    {
        /// <summary>Object description of the source file.</summary>
        IRTFile RtFile { get; set; }

        /// <summary>Generate the output to a file.</summary>
        /// <param name="templatePath">The template to generate from. Pass <c>null</c> if not generating from a template.</param>
        void GenerateFile(string templatePath);

        /// <summary>Generate the output to a string.</summary>
        /// <param name="templatePath">The template to generate from. Pass <c>null</c> if not generating from a template.</param>
        /// <returns>Returns the output source file as string.</returns>
        string Generate(string templatePath);

        /// <summary>Gives the generator an option to specify custom type names for RT CoreType types.</summary>
        /// <param name="mappings">The dictionary with the type mappings (CoreType => Generator type).</param>
        void RegisterTypeMappings(Dictionary<string, string> mappings);
    }
}
