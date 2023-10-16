namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents an element in the documentation comments.</summary>
    /// <remarks>Usually a tag or text.</remarks>
    public interface IDocElement
    {
        /// <summary>Raw text copy of the element in the source comment.</summary>
        string RawText { get; set; }

        /// <summary>The type of the documentation comment element.</summary>
        ElementType ElementType { get; set; }
    }
}
