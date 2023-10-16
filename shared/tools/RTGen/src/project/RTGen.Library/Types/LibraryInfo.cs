using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents the library info for which to generate.</summary>
    public class LibraryInfo : ILibraryInfo
    {
        /// <summary>The library name.</summary>
        /// <example>CoreTypes, CoreObjects</example>
        public string Name { get; set; }

        /// <summary>Library version info (major, minor, patch).</summary>
        public IVersionInfo Version { get; set; }

        /// <summary>The library base namespace.</summary>
        /// <example>Dewesoft::RT::Core</example>
        public INamespace Namespace { get; set; }

        /// <summary>Library output name.</summary>
        /// <example>dscoretypes, dscoreobjects</example>
        public string OutputName { get; set; }
    }
}
