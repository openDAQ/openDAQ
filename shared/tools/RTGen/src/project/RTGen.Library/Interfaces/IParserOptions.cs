using System.Collections.Generic;

namespace RTGen.Interfaces
{
    /// <summary>Additional parser options.</summary>
    public interface IParserOptions
    {
        /// <summary>The declared language of the input file.</summary>
        string InputLanguage { get; set; }

        /// <summary>Whether to stop on first parse error.</summary>
        bool ContinueOnParseErrors { get; set; }

        /// <summary>Library version, name and platform info.</summary>
        ILibraryInfo LibraryInfo { get; set; }

        /// <summary>Predefined type arguments (as macros or constants).</summary>
        IDictionary<string, ISampleTypes> PredefinedTypeArguments { get; set; }

        /// <summary>The CoreTypes namespace override.</summary>
        INamespace CoreNamespace { get; }
    }
}
