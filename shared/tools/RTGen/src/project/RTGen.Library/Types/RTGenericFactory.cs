using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Generic factory function declaration info.</summary>
    [Serializable]
    public class RTGenericFactory : RTFactory, IRTGenericFactory
    {
        /// <summary>Initializes a new instance of the <see cref="RTGenericFactory" /> class.</summary>
        public RTGenericFactory(string prettyName, string interfaceName, IEnumerable<IArgument> args, ISampleTypes genericArgs)
            : base(prettyName, interfaceName, $"create{prettyName}", args)
        {
            GenericTypeArguments = genericArgs;
        }

        private static string CreateFuncName(string prettyName, IReadOnlyCollection<ITypeName> genericArgs)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append($"create{prettyName}");

            if (genericArgs != null && genericArgs.Count > 0)
            {
                sb.Append('_');
                sb.Append(string.Join("_", genericArgs.Select(t => t.UnmappedName)));
            }

            string funcName = sb.ToString();
            return funcName;
        }

        /// <summary>Copies the factory info to a new instance.</summary>
        /// <param name="factory">The factory to copy.</param>
        /// <param name="constructor">Whether to omit the first parameter.</param>
        public RTGenericFactory(IRTGenericFactory factory, bool constructor = false)
            : base(factory, constructor)
        {
            GenericTypeArguments = factory.GenericTypeArguments;
        }

        /// <summary>Copies the factory info to a new instance.</summary>
        /// <param name="factory">The factory to copy.</param>
        /// <param name="kind"></param>
        public RTGenericFactory(IRTFactory factory, IArgument kind)
            : base(factory, kind)
        {
        }

        /// <summary>If the factory has arguments with generic/template types.</summary>
        public override bool IsGeneric => true;

        /// <summary>Factory generic type arguments.</summary>
        /// <example>T, U in <code>createValueAxis&lt;T, U&gt;()</code></example>
        public ISampleTypes GenericTypeArguments { get; }

        /// <summary>Number of predefined specializations</summary>
        public int SpecializationCount => GenericTypeArguments.Count;

        /// <summary>Generate a normal factory for a specific predefined template type.</summary>
        /// <param name="index">Predefined template type index.</param>
        /// <returns>A specialized factory for the templated type</returns>
        public IRTFactorySpecialization Specialize(int index)
        {
            IList<ITypeName> sampleTypes = GenericTypeArguments.Types[index];

            return new RTFactorySpecialization(this, sampleTypes);
        }

        /// <summary>Creates a new object that is a deep copy of the current instance. </summary>
        /// <returns>A new object that is a deep copy of this instance.</returns>
        public override IRTFactory Clone()
        {
            return new RTGenericFactory(this.PrettyName,
                                        this.InterfaceName,
                                        this.Arguments.Select(a => a.Clone()),
                                        GenericTypeArguments)
            {
                Options = this.Options,
            };
        }
    }
}
