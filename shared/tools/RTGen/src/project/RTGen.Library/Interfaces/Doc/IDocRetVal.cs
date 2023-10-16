namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents a function's specific return value with its name and description.</summary>
    public interface IDocRetVal : IDocAttribute
    {
        /// <summary>The name of the specific return value.</summary>
        string ReturnValue { get; }
    }
}
