using RTGen.Interfaces;
using RTGen.Types;

namespace RTGen.Generation
{
    /// <summary>Fallback to basic template generator if no generator for the specified language is found.</summary>
    public class FallbackGenerator : TemplateGenerator
    {
        /// <summary>Generator version info.</summary>
        /// <returns>Returns the generator version info.</returns>
        public override IVersionInfo Version => new VersionInfo
        {
            Major = 1,
            Minor = 0,
            Patch = 0
        };
    }
}
