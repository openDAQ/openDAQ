namespace RTGen.Interfaces
{
    /// <summary>Represents the enumeration option.</summary>
    public interface IEnumOption
    {
        /// <summary>Options name.</summary>
        /// <example>Spade, Clubs, Heart, Diamond.</example>
        string Name { get; set; }

        /// <summary>Enumeration value.</summary>
        /// <example>1, 2, 3, 0xfff, 12345</example>
        string Value { get; set; }

        /// <summary>If enum option has an explicit value (helper).</summary>
        bool HasValue { get; }
    }
}