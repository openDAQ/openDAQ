namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents the information about a generic documentation tag.</summary>
    public interface IDocTag : IDocElement
    {
        /// <summary>The type of the documentation tag if it is a known tag.</summary>
        TagType TagType { get; set; }

        /// <summary>The name of the documentation tag.</summary>
        /// <example>param, brief, returns</example>
        string TagName { get; set; }
    }
}
