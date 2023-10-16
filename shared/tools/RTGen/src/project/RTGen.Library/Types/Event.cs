using RTGen.Interfaces;

namespace RTGen.Types
{
    /// <summary>Represents a single event information.</summary>
    public class Event : IEvent
    {
        /// <summary>Event arguments type (IEventArgs if <c>null</c>).</summary>
        public ITypeName EventArgsType { get; set; }

        /// <summary>Method used to subscribe to the event.</summary>
        public IMethod AddMethod { get; set; }

        /// <summary>Method used to unsubscribe from the event.</summary>
        public IMethod RemoveMethod { get; set; }
    }
}
