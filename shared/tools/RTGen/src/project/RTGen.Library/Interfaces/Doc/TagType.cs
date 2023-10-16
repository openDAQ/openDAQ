namespace RTGen.Interfaces.Doc
{
    /// <summary>Represents a type of a documentation attribute or tag.</summary>
    public enum TagType
    {
        /// <summary>An attribute that has no special recognision or parsing done.</summary>
        Unknown,
        /// <summary>A short description of a documented entity.</summary>
        Brief,
        /// <summary>Describes possible exceptions that can be thrown by the documented entity.</summary>
        Throws,
        /// <summary>Describes a specific parameter of a documented entity.</summary>
        Param,
        /// <summary>References a specific parameter of a documented entity.</summary>
        ParamRef,
        /// <summary>Describes the name and the meaning of a specific return value.</summary>
        RetVal,
        /// <summary>Represents a private section that should not be documented</summary>
        Private,
        /// <summary>Represents a longer and more detailed description than <see cref="Brief"/>.</summary>
        Description,
        /// <summary>Represents a block with open and close.</summary>
        Block
    }
}
