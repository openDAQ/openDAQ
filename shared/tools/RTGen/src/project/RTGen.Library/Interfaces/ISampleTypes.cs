using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RTGen.Interfaces
{
    
    /// <summary>Predefined macro sample types info.</summary>
    public interface ISampleTypes
    {
        /// <summary>Predefined macro name for this sample types.</summary>
        string Name { get; set; }

        /// <summary>Number of generic arguments per item.</summary>
        int Rank { get; }

        /// <summary>Number of predefined types.</summary>
        int Count { get;  }

        /// <summary>Types per item.</summary>
        IList<IList<ITypeName>> Types { get; }
    }
}
