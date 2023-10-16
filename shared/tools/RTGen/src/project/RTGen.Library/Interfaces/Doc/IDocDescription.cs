namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents a longer and more detailed description than <see cref="IDocBrief"/>.</summary>
    public interface IDocDescription : IDocAttribute
    {
        /// <summary>The order of appearance in the documentation comment.</summary>
        int TagIndex { get; }
    }
}
