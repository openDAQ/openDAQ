namespace RTGen.Interfaces
{
    /// <summary>Represents the action the metod does with the event handler (eg. subscribes, unsubscribes etc.).</summary>
    public enum EventMethodType
    {
        /// <summary>Subscribes the event handler to the event.</summary>
        Add,
        /// <summary>Unsubscribes the event handler to the event.</summary>
        Remove
    }
}
