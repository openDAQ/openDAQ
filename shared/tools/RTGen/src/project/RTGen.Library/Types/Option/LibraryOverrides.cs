using System;
using RTGen.Interfaces;

namespace RTGen.Types.Option
{
    [Serializable]
    class LibraryOverrides : Options<string>
    {
        private readonly IAttributeInfo _attributeInfo;

        public LibraryOverrides(IAttributeInfo attributeInfo)
        {
            _attributeInfo = attributeInfo;
        }

        public override bool TryGet(string option, out string value)
        {
            if (!_attributeInfo.IsCoreType(option))
            {
                return base.TryGet(option, out value);
            }

            value = "CoreTypes";
            return true;
        }
    }
}
