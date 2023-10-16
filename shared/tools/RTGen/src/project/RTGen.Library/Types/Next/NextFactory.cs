using System;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;

namespace RTGen.Types
{
    /// <summary>Parsed info about the next factory.</summary>
    [Serializable]
    public class NextFactory
    {
        /// <summary>Additional generation options</summary>
        public FactoryOptions Options { get; set; }

        /// <summary>Documentation comment info.</summary>
        public IDocComment Documentation { get; set; }

        /// <summary>Clears all the info to their defaults.</summary>
        public void Clear()
        {
            Options = FactoryOptions.None;
        }
    }
}
