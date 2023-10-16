namespace RTGen.Interfaces
{
    /// <summary>Wrapper (SmartPtr) info for an interface.</summary>
    public interface IWrapperType
    {
        /// <summary>Type's SmartPtr standard/conventions-based include name.</summary>
        string IncludeName { get; }

        /// <summary>Convention based SmartPtr name. (NonInterfaceName + Suffix)</summary>
        /// <example>IString => StringPtr</example>
        string DefaultName { get; }

        /// <summary>Actual SmartPtr name without namespace and generic parameters.</summary>
        string NameOnly { get; }

        /// <summary>Actual SmartPtr name without namespace.</summary>
        string Name { get; }

        /// <summary>Actual SmartPtr with namespace.</summary>
        string NameFull { get; }

        /// <summary>Uses the SmartPtr name in RT Attributes otherwise falls back to default base SmartPtr.</summary>
        /// <example>If <c>[interfaceSmartPtr(IWidget, WidgetPtr)]</c> is defined => WidgetPtr, otherwise <c>ObjectPtr&lt;IWidget&gt;</c>.</example>
        string BaseName { get; }

        /// <summary>Uses the SmartPtr name with namespace in RT Attributes otherwise falls back to default base SmartPtr.</summary>
        /// <example>If <c>[interfaceSmartPtr(IWidget, WidgetPtr)]</c> is defined => Namespace::WidgetPtr, otherwise <c>Dewesoft::RT::Core::ObjectPtr&lt;IWidget&gt;</c>.</example>
        string BaseNameFull { get; }

        /// <summary>Actual SmartPtr name with namespace but without generic parameters.</summary>
        string NameQualified { get; }
    }
}
