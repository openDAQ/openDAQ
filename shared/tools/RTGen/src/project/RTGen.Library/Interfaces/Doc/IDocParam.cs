namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents a parameter information in the documented entity.</summary>
    public interface IDocParam : IDocAttribute
    {
        /// <summary>The name of the documented parameter.</summary>
        string ParamName { get; }

        /// <summary>Whether this is an output parameter.</summary>
        bool IsOut { get; }
    }
}
