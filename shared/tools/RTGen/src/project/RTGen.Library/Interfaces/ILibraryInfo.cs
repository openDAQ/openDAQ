using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RTGen.Interfaces
{
    /// <summary>Represents the library info for which to generate.</summary>
    public interface ILibraryInfo
    {
        /// <summary>The library name.</summary>
        /// <example>CoreTypes, CoreStructure</example>
        string Name { get; set; }

        /// <summary>Library version info (major, minor, patch).</summary>
        IVersionInfo Version { get; set; }

        /// <summary>The library base namespace.</summary>
        /// <example>Dewesoft::RT::Core</example>
        INamespace Namespace { get; set; }

        /// <summary>Library output name.</summary>
        /// <example>dscoretypes, dscoreobjects</example>
        string OutputName { get; set; }
    }
}
