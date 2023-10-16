using System;
using System.Collections.Generic;
using System.Linq;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents a generic factory specialization.</summary>
    public class RTFactorySpecialization : RTFactory, IRTFactorySpecialization
    {
        /// <summary>Initializes a new instance of the <see cref="RTFactorySpecialization" /> class.</summary>
        public RTFactorySpecialization(IRTGenericFactory factory, IList<ITypeName> sampleTypes)
            : base(factory.PrettyName,
                   factory.InterfaceName,
                   "",
                    GetSpecializedArguments(factory, sampleTypes)
                )
        {
            GenericTypes = sampleTypes;

            FactorySuffix = string.Join("_", sampleTypes.Select(t => t.UnmappedName));
            Name = $"create{PrettyName}_{FactorySuffix}";

            Tag = FactorySuffix;
        }

        private static IEnumerable<IArgument> GetSpecializedArguments(IRTFactory factory, IList<ITypeName> sampleTypes)
        {
            List<IArgument> args = new List<IArgument>(factory.Arguments.Length);
            foreach (IArgument genericParam in factory.Arguments)
            {
                IArgument specializedArg = genericParam;

                if (specializedArg.Type.IsGenericArgument)
                {
                    specializedArg = SpecializeGenericArg(sampleTypes, specializedArg);
                }
                else if (specializedArg.Type.HasGenericArguments)
                {
                    specializedArg = SpecializeTemplatedArg(sampleTypes, specializedArg);
                }

                args.Add(specializedArg);
            }

            return args;
        }

        private static IArgument SpecializeTemplatedArg(IList<ITypeName> sampleTypes, IArgument specializedArg)
        {
            specializedArg = specializedArg.Clone();
            ITypeName genericTypeArg = specializedArg.Type;

            for (int i = 0; i < genericTypeArg.GenericArguments.Count; i++)
            {
                ITypeName type = genericTypeArg.GenericArguments[i];
                if (!type.IsGenericArgument)
                {
                    continue;
                }

                switch (type.UnmappedName)
                {
                    case "T":
                        genericTypeArg.GenericArguments[i] = sampleTypes[0];
                        break;
                    case "U":
                        genericTypeArg.GenericArguments[i] = sampleTypes[1];
                        break;
                    default:
                        throw new NotImplementedException($"Generic type argument {type.UnmappedName} not suported.");
                }

                genericTypeArg.GenericArguments[i].Flags.IsSpecialization = true;
            }

            return specializedArg;
        }

        private static IArgument SpecializeGenericArg(IList<ITypeName> sampleTypes, IArgument specializedArg)
        {
            ITypeName specialzed;
            switch (specializedArg.Type.UnmappedName)
            {
                case "T":
                    specialzed = sampleTypes[0];
                    break;
                case "U":
                    specialzed = sampleTypes[1];
                    break;
                default:
                    throw new NotImplementedException($"Generic type argument {specializedArg.Type.UnmappedName} not suported.");
            }

            specialzed.Flags.IsSpecialization = true;

            return new Argument(specialzed, specializedArg);
        }

        /// <summary>Generic type specifier suffix.</summary>
        public string FactorySuffix { get; }

        /// <summary>Specialization types.</summary>
        public IList<ITypeName> GenericTypes { get; }

        /// <summary>Converts the factory to a plain method.</summary>
        /// <returns>Factory as a method instance.</returns>
        public override IOverload ToOverload(bool constructor = false)
        {
            IOverload overload = base.ToOverload(constructor);
            overload.Tag = GenericTypes;

            return overload;
        }
    }
}
