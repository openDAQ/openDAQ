namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents an element type inside a documentation comment</summary>
    public enum ElementType
    {
        /// <summary>Unknown element type</summary>
        Unknown,
        /// <summary>A simple block of text.</summary>
        Text,
        /// <summary>A documentation attribute/tag with associated data</summary>
        Tag
    }
}
