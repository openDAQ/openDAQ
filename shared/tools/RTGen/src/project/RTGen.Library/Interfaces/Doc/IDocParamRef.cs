namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents an inline documentation attribute inside a description that refers to another parameter.</summary>
    public interface IDocParamRef : IDocTag
    {
        /// <summary>The name of the referenced parameter.</summary>
        string ParamName { get; }
    }
}
