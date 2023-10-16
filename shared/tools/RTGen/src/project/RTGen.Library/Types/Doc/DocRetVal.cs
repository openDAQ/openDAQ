using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents a function's specific return value with its name and description.</summary>
    public class DocRetVal : DocAttribute, IDocRetVal
    {
        /// <summary>Initializes a new instance of the <see cref="DocRetVal" /> class.</summary>
        /// <param name="returnValue">The name of the return value.</param>
        /// <param name="rawText">The raw copy of the source text for the element.</param>
        public DocRetVal(string returnValue, string rawText) : base("retval", TagType.RetVal, rawText)
        {
            ReturnValue = returnValue;
        }

        /// <summary>The name of the specific return value.</summary>
        public string ReturnValue { get; set; }
    }
}
