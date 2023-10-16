using System;
using RTGen.Interfaces;
using RTGen.Util;

namespace RTGen.Types.Option
{
    /// <summary>Custom interface to SmartPtr mappings.</summary>
    [Serializable]
    public class PtrMappings : Options<ISmartPtr>
    {
        /// <summary>Adds or modifies the specified <paramref name="option"/> value.</summary>
        /// <param name="option">The option name.</param>
        /// <param name="value">The option value.</param>
        public override void Add(string option, ISmartPtr value)
        {
            if (TryGet(option, out ISmartPtr ptr))
            {
                Log.Warning($"Merging SmartPtr overrides for {option}.");

                if (!string.IsNullOrEmpty(value.Name))
                {
                    ptr.Name = value.Name;
                }

                if (string.IsNullOrEmpty(ptr.DefaultAliasName))
                {
                    ptr.DefaultAliasName = value.DefaultAliasName;
                }
            }
            else
            {
                base.Add(option, value);
            }
        }
    }
}
