using System.Runtime.Serialization;
using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents the get/set property pair.</summary>
    /// <remarks>Nothing to do with PropertyObject.</remarks>
    public class GetSet : IGetSet
    {
        /// <summary>Creates a new instance of GetSet pair with the specified <paramref name="name"/>.</summary>
        /// <param name="name">The get/set property name.</param>
        public GetSet(string name)
        {
            Name = name;
        }

        /// <summary>Creates a property with the specified getter.</summary>
        /// <param name="name">The name of the property pair.</param>
        /// <param name="getter">The getter function.</param>
        /// <returns>The get/set pair with the getter predefined.</returns>
        public static IGetSet AsGetter(string name, IMethod getter)
        {
            return new GetSet(name)
            {
                Getter = getter
            };
        }

        /// <summary>Creates a property with the specified setter.</summary>
        /// <param name="name">The name of the property pair.</param>
        /// <param name="setter">The setter function.</param>
        /// <returns>The get/set pair with the setter predefined.</returns>
        public static IGetSet AsSetter(string name, IMethod setter)
        {
            return new GetSet(name)
            {
                Setter = setter
            };
        }

        /// <summary>The name of the get/set property pair.</summary>
        public string Name { get; set; }

        /// <summary>The getter part of the property.</summary>
        public IMethod Getter { get; set; }

        /// <summary>The setter part of the property.</summary>
        public IMethod Setter { get; set; }
    }
}
