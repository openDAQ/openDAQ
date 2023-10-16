using System;
using RTGen.Interfaces;
using RTGen.Interfaces.Doc;

namespace RTGen.Types
{
    /// <summary>Parsed info about the next method.</summary>
    [Serializable]
    public class NextMethod : PropertyHolder
    {

        /// <summary>Creates a default initialized instance.</summary>
        public NextMethod()
        {
            Arguments = new Options<IArgumentInfo>();
        }

        /// <summary>If not <c>null</c> next apply defaults to property method.</summary>
        public IProperty PropertyInfo { get; set; }

        /// <summary>The event info for the next method, otherwise <c>null</c>.</summary>
        public MethodEventInfo EventInfo { get; set; }

        /// <summary>If a certain kind of generator should ignore this method.</summary>
        public GeneratorType IsIgnored { get; set; }

        /// <summary>If a method should return the object instead of null.</summary>
        public bool ReturnSelf { get; set; }

        /// <summary>Additional info about method's arguments.</summary>
        public IOptions<IArgumentInfo> Arguments { get; set; }

        /// <summary>Documentation comment info for the method.</summary>
        public IDocComment Documentation { get; set; }

        /// <summary>Clear the next method info.</summary>
        public void Reset()
        {
            PropertyInfo = null;
            EventInfo = null;
            Documentation = null;
            IsIgnored = GeneratorType.None;
            ReturnSelf = false;
            Arguments.Clear();

            ClearProperties();
        }
    }
}
