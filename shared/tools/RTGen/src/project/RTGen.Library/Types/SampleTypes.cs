using System;
using System.Collections.Generic;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Predefined macro sample types info.</summary>
    public class SampleTypes : ISampleTypes
    {
        private int _typeCounter;

        /// <summary>Creates a new instance of <see cref="SampleTypes"/>.</summary>
        /// <param name="name">The name of the predefined macro.</param>
        /// <param name="rank">The number of types per item.</param>
        public SampleTypes(string name, int rank)
        {
            _typeCounter = rank - 1;

            Name = name;
            Rank = rank;
            Types = new List<IList<ITypeName>>();
        }

        /// <summary>Predefined macro name for this sample types.</summary>
        public string Name { get; set; }

        /// <summary>Number of generic arguments per item.</summary>
        public int Rank { get; private set; }

        /// <summary>Types per item.</summary>
        public IList<IList<ITypeName>> Types { get; private set; }

        /// <summary>Number of predefined types.</summary>
        public int Count => Types.Count;

        /// <summary>Add additional item type</summary>
        /// <param name="typeName">The specialization item type.</param>
        public void AddType(ITypeName typeName)
        {
            if (++_typeCounter % Rank == 0)
            {
                Types.Add(new List<ITypeName>{typeName});
            }
            else
            {
                Types[Count - 1].Add(typeName);
            }
        }
    }
}
