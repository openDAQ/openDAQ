using System;
using System.Collections.Generic;

namespace RTGen.Types
{
    [Serializable]
    internal class ValueType : Options<bool>
    {
        /// <summary>A list of RT CoreTypes value types.</summary>
        internal static readonly HashSet<string> RTValueTypes = new HashSet<string>
        {
            "Bool",
            "Int",
            "Float",
            "SizeT",
            "EnumType",
            "uint32_t",
            "CoreType"
        };

        public override bool Has(string option)
        {
            return base.Has(option) || RTValueTypes.Contains(option);
        }

        public override bool Get(string option)
        {
            if (base.Has(option))
            {
                return base.Get(option);
            }

            return Has(option);
        }

        public override bool TryGet(string option, out bool value)
        {
            if (base.TryGet(option, out value))
            {
                return true;
            }

            if (RTValueTypes.Contains(option))
            {
                value = true;
                return true;
            }
            return false;
        }
    }
}
