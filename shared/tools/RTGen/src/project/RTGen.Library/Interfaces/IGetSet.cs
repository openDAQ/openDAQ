namespace RTGen.Interfaces
{
    /// <summary>Represents the get/set property pair.</summary>
    /// <remarks>Nothing to do with PropertyObject.</remarks>
    public interface IGetSet
    {
        /// <summary>The name of the get/set property pair.</summary>
        string Name { get; set; }

        /// <summary>The getter part of the property.</summary>
        IMethod Getter { get; set; }

        /// <summary>The setter part of the property.</summary>
        IMethod Setter { get; set; }
    }
}
