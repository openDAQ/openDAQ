using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents an interface template specialization.</summary>
    public class Specialization
    {
        /// <summary>Creates a new instance of interface specialization info.</summary>
        /// <param name="intf">The interface to specialize.</param>
        /// <param name="types">The specialization type parameters.</param>
        public Specialization(IRTInterface intf, IList<ITypeName> types)
        {
            Inteface = intf;
            Types = types;
        }

        /// <summary>Interface info.</summary>
        public IRTInterface Inteface { get; private set; }

        /// <summary>List of explicit template types.</summary>
        public IList<ITypeName> Types { get; private set; }
    }
}
