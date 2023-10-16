namespace RTGen.Interfaces
{
    /// <summary>Represents an RT Attribute argument.</summary>
    public interface IRTAttributeArgument
    {
        /// <summary>Name of the parameter if using named parameters.</summary>
        /// <example>[property(default: 0)] => default</example>
        string Name { get; }

        /// <summary>Whether the parameter is named.</summary>
        /// <example>[property(default: 0)] => true, [property(0)] => false</example>
        bool IsNamedParameter { get; }

        /// <summary>The value of the parameter.</summary>
        /// <example>[property(default: 0)] => "0"</example>
        string Value { get; }

        /// <summary>Argument info if attribute parameters are specified in the form of {type} {name}. Otherwise <c>null</c>.</summary>
        IArgument Type { get; }

        ITypeName TypeInfo { get; }
    }
}
