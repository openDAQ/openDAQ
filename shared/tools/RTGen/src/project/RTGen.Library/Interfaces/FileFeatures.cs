using System;

namespace RTGen.Interfaces
{
    /// <summary>Represents additional features in the interface file.</summary>
    [Flags]
    public enum FileFeatures
    {
        /// <summary>Without special features.</summary>
        None = 0x00,
        /// <summary>File has events that need to be wrapped.</summary>
        Events = 0x01,
        /// <summary>File has MUI controls.</summary>
        Controls = 0x02,
        /// <summary>File methods that take or return arrays by pointer.</summary>
        Span = 0x04
    }
}
