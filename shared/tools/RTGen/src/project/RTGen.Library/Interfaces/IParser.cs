namespace RTGen.Interfaces
{
    /// <summary>What the parser is parsing.</summary>
    public enum ParsedFile
    {
        /// <summary>Normal interface files.</summary>
        Interface,
        /// <summary>Custom file (not required to contain an interface).</summary>
        Custom,
    }

    /// <summary>Parses the source file into an object representation used.</summary>
    public interface IParser
    {
        /// <summary>Parse the specified file into an object representation as specified in options</summary>
        /// <param name="fileName">The filename to parse.</param>
        /// <param name="options">Additional options.</param>
        /// <returns>An object representation of the specified source file.</returns>
        IRTFile Parse(string fileName, IParserOptions options);

        /// <summary>What the parser is parsing.</summary>
        ParsedFile FileType { get; }
    }
}
