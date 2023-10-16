using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents a parameter information in the documented entity.</summary>
    public class DocParam : DocAttribute, IDocParam
    {
        /// <summary>Initializes a new instance of the <see cref="DocParam" /> class.</summary>
        /// <param name="paramName">The name of the documented parameter.</param>
        /// <param name="isOut">Whether this is an output parameter.</param>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        public DocParam(string paramName, bool isOut, string rawText) : base("param", TagType.Param, rawText)
        {
            IsOut = isOut;
            ParamName = paramName;
        }

        /// <summary>The name of the documented parameter.</summary>
        public string ParamName { get; set; }

        /// <summary>Whether this is an output parameter.</summary>
        public bool IsOut { get; set; }
    }
}
