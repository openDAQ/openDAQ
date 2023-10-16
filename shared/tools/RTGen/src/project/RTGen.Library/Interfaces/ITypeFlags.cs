namespace RTGen.Interfaces
{
    /// <summary>Contains type flags and options.</summary>
    public interface ITypeFlags : IOptions<bool>
    {

        /// <summary>If the next type should have a decorator generated.</summary>
        bool IsDecorated { get; }

        /// <summary>If the SmartPtr is generic/templated.</summary>
        bool IsGeneric { get; }

        /// <summary>If the type is a RT CoreTypes type</summary>
        bool IsCoreType { get; }

        /// <summary>Whether the type represents a UI Control type.</summary>
        bool IsUiControl { get; }

        /// <summary>Checks if the type if value-type.</summary>
        /// <returns>Returns <c>true</c> if the type is value-type otherwise <c>false</c>.</returns>
        bool IsValueType { get; }

        /// <summary>Checks if the type is specialization of a generic/templated type.</summary>
        /// <returns>Returns <c>true</c> if the type is a specialization otherwise <c>false</c>.</returns>
        bool IsSpecialization { get; set; }

        /// <summary>If is an SDK internal configuration object.</summary>
        /// <example>`ISignalConfig` is a config object for `ISignal`</example>
        bool IsCoreConfig { get; }
    }
}
