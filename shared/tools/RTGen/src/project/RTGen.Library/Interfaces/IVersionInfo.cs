
namespace RTGen.Interfaces
{
    /// <summary>Represents IGenerator and IParser versions.</summary>
    public interface IVersionInfo
    {
        /// <summary>Increments on a breaking change.</summary>
        int Major { get; set; }

        /// <summary>Increments when a feature was added (and possible bugfixes).</summary>
        int Minor { get; set; }

        /// <summary>Increments when only bugfixes were made.</summary>
        int Patch { get; set; }
    }
}
