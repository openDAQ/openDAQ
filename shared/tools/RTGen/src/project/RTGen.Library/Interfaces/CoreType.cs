
namespace RTGen.Interfaces
{
    /// <summary>Enumeration of RT Core base types.</summary>
    public enum CoreType
    {
        /// <summary>Boolean.</summary>
        Bool,
        /// <summary>Integer 64bit</summary>
        Int,
        /// <summary>Float 64bit</summary>
        Float,
        /// <summary>String</summary>
        String,
        /// <summary>List</summary>
        List,
        /// <summary>Dictionary / HashMap</summary>
        Dictionary,
        /// <summary>Timebase / Rational number</summary>
        Ratio,
        /// <summary>Procedure / Callback</summary>
        Procedure,
        /// <summary>Object</summary>
        Object,
        /// <summary>Binary blob</summary>
        BinaryData,
        /// <summary>Function</summary>
        Function,
        /// <summary>Iterable</summary>
        Iterable,
        /// <summary>Unknown or undefined</summary>
        Undefined,
    }
}
