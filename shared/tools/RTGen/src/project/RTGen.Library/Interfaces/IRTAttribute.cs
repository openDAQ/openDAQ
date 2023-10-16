namespace RTGen.Interfaces
{
    /// <summary>Represents Attribute info.</summary>
    public interface IRTAttribute
    {
        /// <summary>Attribute name.</summary>
        /// <example>includeHeader</example>
        string Name { get; set; }

        /// <summary>Attribute arguments (unparsed, as read from file).</summary>
        IRTAttributeArgument[] Arguments { get; set; }
    }
}
