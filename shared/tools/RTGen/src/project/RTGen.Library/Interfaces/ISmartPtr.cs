namespace RTGen.Interfaces
{
    /// <summary>Represents SmartPtr info.</summary>
    public interface ISmartPtr
    {
        /// <summary>Smart-pointer name.</summary>
        /// <example>StringPtr, ObjectPtr.</example>
        string Name { get; set; }

        /// <summary>Whether the smart-pointer is templated (generic).</summary>
        /// <example>EventArgsPtr&lt;Interface&gt;.</example>
        bool IsTemplated { get; set; }

        /// <summary>Whether to generated default template using/typedef.</summary>
        /// <example>using Control = ControlPtr&lt;&gt;;</example>
        bool HasDefaultAlias { get; set; }

        /// <summary>The name of the default template using/typedef it the SmartPtr is templated.</summary>
        string DefaultAliasName { get; set; }

        /// <summary>Whether to generate InterfaceToSmartPtr&lt;&gt; specialization.</summary>
        bool IsDefaultPtr { get; set; }
    }
}
