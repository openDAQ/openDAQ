using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using RTGen.Interfaces;

namespace RTGen.Util
{
    /// <summary>Provides an Equality comparer for IRTInterface.</summary>
    public class RtFactoryComparer : IEqualityComparer<IRTFactory>
    {
        /// <summary>Determines whether the specified objects are equal.</summary>
        /// <returns>true if the specified objects are equal; otherwise, false.</returns>
        /// <param name="x">The first object of type <see cref="IRTFactory"/> to compare.</param>
        /// <param name="y">The second object of type <see cref="IRTFactory"/> to compare.</param>
        public bool Equals(IRTFactory x, IRTFactory y)
        {
            // Check reference equality
            if (x == y)
            {
                return true;
            }

            return x != null && x.Equals(y);
        }

        /// <summary>Returns a hash code for the specified object.</summary>
        /// <returns>A hash code for the specified object.</returns>
        /// <param name="obj">The <see cref="T:RTGen.Interfaces.IRTFactory" /> for which a hash code is to be returned.</param>
        /// <exception cref="T:System.ArgumentNullException">The type of <paramref name="obj" /> is a reference type and <paramref name="obj" /> is null.</exception>
        public int GetHashCode(IRTFactory obj)
        {
            return obj.Name.GetHashCode();
        }
    }
}
