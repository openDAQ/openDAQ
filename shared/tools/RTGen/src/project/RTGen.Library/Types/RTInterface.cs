using System.Collections.Generic;
using System.Linq;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;
using RTGen.Util;

namespace RTGen.Types
{
    /// <summary>Represents a RT interface info.</summary>
    public class RTInterface : IRTInterface
    {
        /// <summary>Initializes a new instance of the <see cref="RTInterface" /> class.</summary>
        public RTInterface()
        {
            Methods = new List<IMethod>();
            GetSet = new List<IGetSet>();
            Events = new Dictionary<string, IEvent>();
        }

        /// <summary>Interface type name info.</summary>
        public ITypeName Type { get; set; }

        /// <summary>Base interface type name info.</summary>
        public ITypeName BaseType { get; set; }

        /// <summary>Interface methods info.</summary>
        public IList<IMethod> Methods { get; set; }

        /// <summary>Interface get/set pairs.</summary>
        public IList<IGetSet> GetSet { get; set; }

        /// <summary>A map of events information.</summary>
        public IDictionary<string, IEvent> Events { get; set; }

        /// <summary>Documentation comments info for the interface.</summary>
        public IDocComment Documentation { get; set; }

        /// <summary>Creates a new object that is a deep copy of the current instance. </summary>
        /// <returns>A new object that is a deep copy of this instance.</returns>
        public IRTInterface Clone()
        {
            return new RTInterface
            {
                Type = this.Type.Clone(),
                BaseType = this.Type.Clone(),
                Methods = this.Methods.Select(m => m.Clone()).ToList(),
                Events = this.Events,
                Documentation = null
            };
        }

        /// <summary>Human representation.</summary>
        public override string ToString()
        {
            return Type != null
                       ? Type.ToString()
                       : "Unknown Class";
        }
    }
}
