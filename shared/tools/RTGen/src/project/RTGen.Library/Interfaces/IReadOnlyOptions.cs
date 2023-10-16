using System.Collections.Generic;
using System.Runtime.CompilerServices;

namespace RTGen.Interfaces
{
    /// <summary>Represents a read-only view of the section options.</summary>
    /// <typeparam name="T">The value type of the options.</typeparam>
    public interface IReadOnlyOptions<T> : IEnumerable<KeyValuePair<string, T>>
    {
        /// <summary>Checks if the specified <paramref name="option"/> is available.</summary>
        /// <param name="option">The option to check for.</param>
        /// <returns>Returns <c>true</c> if the option is available otherwise <c>false</c>.</returns>
        bool Has(string option);

        /// <summary>Retrieves the specified <paramref name="option"/> value.</summary>
        /// <param name="option">The option for which to get the value.</param>
        /// <returns>Returns option's value otherwise throws an exception.</returns>
        T Get(string option);

        /// <summary>Retrieves the specified <paramref name="option"/> value.</summary>
        /// <param name="option">The option for which to get the value.</param>
        T this[string option] { get; }

        /// <summary>Tries to retrieve the specified <paramref name="option"/> value.</summary>
        /// <param name="option">The option for which to get the value.</param>
        /// <param name="value">The option value if the available.</param>
        /// <returns>Returns <c>true</c> if the option was retrieved successfully, otherwise <c>false</c>.</returns>
        bool TryGet(string option, out T value);
    }
}
