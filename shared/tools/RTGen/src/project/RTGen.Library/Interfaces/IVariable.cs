namespace RTGen.Interfaces
{
    /// <summary>Represents a variable info.</summary>
    public interface IVariable
    {
        /// <summary>Variable type info.</summary>
        ITypeName Type { get; set; }

        /// <summary>Variable name.</summary>
        string Name { get; set; }

        /// <summary>Unparsed value (e.g. in C++ between <c>=</c> or <c>:=</c> in Delphi and <c>;</c>).</summary>
        string Value { get; set; }
    }
}