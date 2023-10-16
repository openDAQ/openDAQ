using RTGen.Interfaces;

namespace RTGen.Attributes
{
    /// <summary>Represents [propertyClass] attribute parser.</summary>
    public class PropertyClassAttribute : AttributeParser<IPropertyClass>
    {
        private static readonly string[] ArgumentNames =
        {
            "implBase",
            "implTemplated",
            "className",
            "parent",
            "implTemplate"
        };

        /// <summary>Initializes the PropertyClassAttribute parser.</summary>
        /// <param name="propClass">The property class to fill with parsed values.</param>
        public PropertyClassAttribute(IPropertyClass propClass) : base(propClass, ArgumentNames)
        {
        }

        /// <summary>Parses the parameter value and sets the model accordingly.</summary>
        /// <param name="paramName">The name of the parameter.</param>
        /// <param name="value">The parameter value.</param>
        protected override void ParseParameter(string paramName, string value)
        {
            switch (paramName)
            {
                case "className":
                    Model.ClassName = value;
                    break;
                case "parent":
                    Model.ParentClassName = value;
                    break;
                case "implBase":
                    Model.ImplementationBase = value.Trim('"');
                    break;
                case "implTemplated":
                    Model.IsImplementationTemplated = ParseBoolOrThrow(value, paramName);
                    break;
                case "implTemplate":
                    Model.ImplementationTemplate = value.Trim('"');
                    break;
                case "privateBase":
                    Model.PrivateImplementation = ParseBoolOrThrow(value, paramName);
                    break;
            }
        }
    }
}
