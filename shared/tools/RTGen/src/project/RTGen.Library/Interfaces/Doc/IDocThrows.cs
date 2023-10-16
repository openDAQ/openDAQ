namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents the documentation attribute declaring a possible exception.</summary>
    public interface IDocThrows
    {
        /// <summary>The name of the specified exception.</summary>
        string ExceptionName { get; }
    }
}
