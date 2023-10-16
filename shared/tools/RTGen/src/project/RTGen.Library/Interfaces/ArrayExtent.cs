namespace RTGen.Interfaces
{
    /// <summary>Represents how the array size is expressed</summary>
    public enum ArrayExtent
    {
        /// <summary>Unknown array size</summary>
        Unknown,
        /// <summary>Array size given as an explicit literal number</summary>
        Explicit,
        /// <summary>Array size given as a separate parameter</summary>
        Parameter,
        /// <summary>Array size given as a template parameter</summary>
        TemplateParameter
    }
}
