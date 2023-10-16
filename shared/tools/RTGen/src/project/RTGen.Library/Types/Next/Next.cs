using System;

namespace RTGen.Types
{
    /// <summary>The next symbol attribute info.</summary>
    [Serializable]
    public class Next : PropertyHolder
    {
        /// <summary>Initializes the object.</summary>
        public Next()
        {
            Type = new NextType();
            Method = new NextMethod();
            Factory = new NextFactory();
        }

        /// <summary>Next type attribute info.</summary>
        public NextType Type { get; }

        /// <summary>Next method attribute info.</summary>
        public NextMethod Method { get; }

        /// <summary>If the next header include found should be in the output as-well.</summary>
        public bool AddHeader { get; set; }

        /// <summary>Next factory attribute info.</summary>
        public NextFactory Factory { get; }
    }
}
