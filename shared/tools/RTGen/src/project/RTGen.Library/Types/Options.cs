using System;
using System.Collections;
using System.Collections.Generic;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents a modifiable view of the section options.</summary>
    [Serializable]
    public class Options<T> : IOptions<T>
    {
        /// <summary>Backing store for property values.</summary>
        private readonly Dictionary<string, T> _values;

        /// <summary>Initializes the section options.</summary>
        /// <param name="values">Predefined option values.</param>
        public Options(Dictionary<string, T> values)
        {
            _values = values;
        }

        /// <summary>Initializes the section options.</summary>
        public Options() : this(new Dictionary<string, T>())
        {
        }

        /// <summary>Adds or modifies the specified <paramref name="option"/> value.</summary>
        /// <param name="option">The option name.</param>
        /// <param name="value">The option value.</param>
        public virtual void Add(string option, T value)
        {
            _values.Add(option, value);
        }

        /// <summary>Checks if the specified <paramref name="option"/> is available.</summary>
        /// <param name="option">The option to check for.</param>
        /// <returns>Returns <c>true</c> if the option is available otherwise <c>false</c>.</returns>
        public virtual bool Has(string option)
        {
            return _values.ContainsKey(option);
        }

        /// <summary>Retrieves the specified <paramref name="option"/> value.</summary>
        /// <param name="option">The option for which to get the value.</param>
        /// <returns>Returns option's value otherwise throw an exception.</returns>
        public virtual T Get(string option)
        {
            return _values[option];
        }

        /// <summary>Retrieves the specified <paramref name="option"/> value.</summary>
        /// <param name="option">The option for which to get the value.</param>
        public T this[string option] => _values[option];

        /// <summary>Tries to retrieve the specified <paramref name="option"/> value.</summary>
        /// <param name="option">The option for which to get the value.</param>
        /// <param name="value">The option value if the available.</param>
        /// <returns>Returns <c>true</c> if the option was retrieved successfully, otherwise <c>false</c>.</returns>
        public virtual bool TryGet(string option, out T value)
        {
            return _values.TryGetValue(option, out value);
        }

        /// <summary>Clear all options in this section.</summary>
        public virtual void Clear()
        {
            _values.Clear();
        }

        /// <summary>Clear the specified <paramref name="option"/> in this section.</summary>
        /// <param name="option">The option name.</param>
        /// <returns>Returns <c>true</c> if clearing was successful otherwise <c>false</c>.</returns>
        public virtual bool ClearOption(string option)
        {
            return _values.Remove(option);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        /// <summary>Returns an enumerator that iterates through the collection.</summary>
        /// <returns>An enumerator that can be used to iterate through the collection.</returns>
        public virtual IEnumerator<KeyValuePair<string, T>> GetEnumerator()
        {
            return _values.GetEnumerator();
        }
    }
}
