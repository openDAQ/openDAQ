using System.Collections.Generic;

namespace RTGen.Interfaces
{
    /// <summary>Represents a generic factory specialization.</summary>
    public interface IRTFactorySpecialization : IRTFactory
    {
        /// <summary>Generic type specifier suffix.</summary>
        string FactorySuffix { get; }

        /// <summary>Specialization types.</summary>
        IList<ITypeName> GenericTypes { get; }
    }
}
