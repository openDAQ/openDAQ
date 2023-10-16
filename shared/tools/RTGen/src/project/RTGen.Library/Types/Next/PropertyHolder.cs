using System;
using System.Collections.Generic;

namespace RTGen.Types
{
    /// <summary>Represents a generic dynamic property object.</summary>
    [Serializable]
    public class PropertyHolder
    {
        private readonly Dictionary<string, object> _properties;

        /// <summary>Initializes the object.</summary>
        public PropertyHolder()
        {
            _properties = new Dictionary<string, object>();
        }

        /// <summary>Checks if the specified <paramref name="property"/> exists.</summary>
        /// <param name="property">The property to check for.</param>
        /// <returns>Returns <c>true</c> if the property exists otherwise <c>false</c>.</returns>
        public bool Has(string property)
        {
            return _properties.ContainsKey(property);
        }

        /// <summary>Tries to retrieve the specified <paramref name="property"/> value.</summary>
        /// <param name="property">The property for which to get the value.</param>
        /// <param name="value">The property value if the available.</param>
        /// <returns>Returns <c>true</c> if the property was retrieved successfully, otherwise <c>false</c>.</returns>
        public bool TryGet<T>(string property, out T value)
        {
            if (_properties.TryGetValue(property, out object obj))
            {
                value = (T) obj;
                return true;
            }

            value = default(T);
            return false;
        }

        /// <summary>Retrieves the specified <paramref name="property"/> value and casts it to the provided type.</summary>
        /// <param name="property">The property value to retrieve.</param>
        /// <typeparam name="T">The type to cast to.</typeparam>
        /// <returns>Returns the property value otherwise throws an exception if the property is not found or conversion failed.</returns>
        public T Get<T>(string property)
        {
            return (T) _properties[property];
        }

        /// <summary>Sets or modifies the specified <paramref name="property"/> value.</summary>
        /// <param name="property">The property to set.</param>
        /// <param name="value">The property value.</param>
        /// <typeparam name="T">The type of the property value.</typeparam>
        public void Set<T>(string property, T value)
        {
            _properties.Add(property, value);
        }

        /// <summary>Removes all properties.</summary>
        protected void ClearProperties()
        {
            _properties.Clear();
        }
    }
}
