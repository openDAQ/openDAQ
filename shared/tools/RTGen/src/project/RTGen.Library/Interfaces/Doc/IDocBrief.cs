namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents a short description of a documented entity.</summary>
    public interface IDocBrief : IDocAttribute
    {
        /// <summary>The order of appearance in the documentation comment.</summary>
        int TagIndex { get; }
    }
}
