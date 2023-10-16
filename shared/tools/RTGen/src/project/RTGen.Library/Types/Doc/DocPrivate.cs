using RTGen.Interfaces.Doc;

namespace RTGen.Types.Doc
{
    /// <summary>Represents a "private" documentation attribute.</summary>
    public class DocPrivate : DocTag, IDocPrivate
    {

        /// <summary>Initializes a new instance of the <see cref="DocPrivate" /> class.</summary>
        public DocPrivate() : base("private", TagType.Private, string.Empty)
        {
        }
    }
}
