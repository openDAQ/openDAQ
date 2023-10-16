namespace RTGen.Interfaces
{
    /// <summary>Represents a modifiable view of the section options.</summary>
    public interface IOptions<T> : IReadOnlyOptions<T>
    {
        /// <summary>Adds or modifies the specified <paramref name="option"/> value.</summary>
        /// <param name="option">The option name.</param>
        /// <param name="value">The option value.</param>
        void Add(string option, T value);

        /// <summary>Clear all options in this section.</summary>
        void Clear();

        /// <summary>Clear the specified <paramref name="option"/> in this section.</summary>
        /// <param name="option">The option name.</param>
        /// <returns>Returns <c>true</c> if clearing was successful otherwise <c>false</c>.</returns>
        bool ClearOption(string option);
    }
}
