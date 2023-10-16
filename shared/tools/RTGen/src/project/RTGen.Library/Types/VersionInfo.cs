using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents IGenerator and IParser versions.</summary>
    public class VersionInfo : IVersionInfo
    {
        /// <summary>Increments on a breaking change.</summary>
        public int Major { get; set; }

        /// <summary>Increments when a feature was added (and possible bugfixes).</summary>
        public int Minor { get; set; }

        /// <summary>Increments when only bugfixes were made.</summary>
        public int Patch { get; set; }
    }
}
