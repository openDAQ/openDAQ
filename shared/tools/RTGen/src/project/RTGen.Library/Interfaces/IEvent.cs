namespace RTGen.Interfaces
{
    /// <summary>Represents an event info.</summary>
    public interface IEvent
    {
        /// <summary>Event arguments type (IEventArgs if <c>null</c>).</summary>
        ITypeName EventArgsType { get; set; }

        /// <summary>Method used to subscribe to the event.</summary>
        IMethod AddMethod { get; set; }

        /// <summary>Method used to unsubscribe from the event.</summary>
        IMethod RemoveMethod { get; set; }
    }
}
