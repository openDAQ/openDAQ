using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using RTGen.Interfaces.Doc;

namespace RTGen.Interfaces
{
    /// <summary>Factory option.</summary>
    [Flags]
    public enum FactoryOptions
    {
        /// <summary>No special options.</summary>
        None = 0x0,
        /// <summary>Don't generate a constructor overload.</summary>
        NoConstructor = 0x1,
        /// <summary>Generate a wrapped factory function overload.</summary>
        Wrap = 0x2,
        /// <summary>Do not generate bindings for this factory.</summary>
        Hide = 0x4,
        /// <summary>Hide base factory declaration.</summary>
        NoDeclaration = 0x8,
        /// <summary>Hide base factory and only expose the wrapped factory.</summary>
        WrapperOnly = NoDeclaration | NoConstructor | Wrap
    }

    /// <summary>Factory function declaration info.</summary>
    public interface IRTFactory : IEquatable<IRTFactory>,
                                  ICloneable<IRTFactory>
    {
        /// <summary>The identifier name of the factory function.</summary>
        string Name { get; }

        /// <summary>Factory pretty name.</summary>
        string PrettyName { get; }

        /// <summary>Factory tag if multiple factories have the same parameters and types.</summary>
        string Tag { get; set; }

        /// <summary>Whether this is a disambiguated constructor using multiple factories.</summary>
        bool IsConstructorWithMultipleFactories { get; }

        /// <summary>Factory return interface name.</summary>
        string InterfaceName { get; }

        /// <summary>Factory function argument info.</summary>
        IArgument[] Arguments { get; }

        /// <summary>Converts the factory to a plain method.</summary>
        /// <returns>Factory as a method instance.</returns>
        IOverload ToOverload(bool constructor = false);

        /// <summary>Additional generation options</summary>
        FactoryOptions Options { get; set; }

        /// <summary>If the factory has arguments with generic/template types.</summary>
        bool IsGeneric { get; }

        /// <summary>Documentation comment info.</summary>
        IDocComment Documentation { get; }
    }

    /// <summary>Generic factory function declaration info.</summary>
    public interface IRTGenericFactory : IRTFactory
    {
        /// <summary>Factory generic type arguments.</summary>
        /// <example>T, U in <code>createValueAxis&lt;T, U&gt;()</code></example>
        ISampleTypes GenericTypeArguments { get; }

        /// <summary>Number of predefined specializations</summary>
        int SpecializationCount { get; }

        /// <summary>Generate a normal factory for a specific predefined template type.</summary>
        /// <param name="index">Predefined template type index.</param>
        /// <returns>A specialized factory for the templated type</returns>
        IRTFactorySpecialization Specialize(int index);
    }   
}
