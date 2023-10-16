using System;
using RTGen.Interfaces;

namespace RTGen.Types {

    /// <summary>Represent information about the event the method is manipulating.</summary>
    [Serializable]
    public class MethodEventInfo
    {
        /// <summary>The event name.</summary>
        /// <example>OnClick</example>
        public string EventName { get; set; }

        /// <summary>Method type (add event, remove event etc.).</summary>
        public EventMethodType MethodType { get; set; }

        /// <summary>Method type (add / remove event listener etc.).</summary>
        public ITypeName EventArgsType { get; set; }
    }
}
