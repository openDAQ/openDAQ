using System.Collections.Generic;
using RTGen.Interfaces.Doc;

namespace RTGen.Interfaces
{
    /// <summary>Represents a RT interface info.</summary>
    public interface IRTInterface : ICloneable<IRTInterface>
    {
        /// <summary>Interface type name info.</summary>
        ITypeName Type { get; set; }

        /// <summary>Base/inherited interface type name info.</summary>
        ITypeName BaseType { get; set; }

        /// <summary>Interface methods info.</summary>
        IList<IMethod> Methods { get; set; }

        /// <summary>A map of events information.</summary>
        IDictionary<string, IEvent> Events { get; set; }

        /// <summary>Documentation comments info for the interface.</summary>
        IDocComment Documentation { get; set; }

        /// <summary>Interface get/set pairs.</summary>
        IList<IGetSet> GetSet { get; set;  }
    }
}
