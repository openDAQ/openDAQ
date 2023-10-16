using System;
using RTGen.Exceptions;
using RTGen.Interfaces;

namespace RTGen.Attributes
{
    /// <summary>Represents PropertyAttribute parser.</summary>
    public class PropertyAttribute : AttributeParser<IProperty>
    {
        private static readonly string[] ArgumentNames =
        {
            "kind",
            "name",
            "value",
            "defaultValue",
            "values",
            "enabled",
            "visibleOnlyThroughRef",
            "visible",
            "readOnly",
            "readOnlyValue",
            "fieldName",
            "staticConfAction",
            "noField"
        };

        /// <summary>Initializes the PropertyAttribute parser.</summary>
        /// <param name="property">The property to fill with parsed values.</param>
        public PropertyAttribute(IProperty property) : base(property, ArgumentNames)
        {
        }

        /// <summary>Parses the parameter value and sets the model accordingly.</summary>
        /// <param name="paramName">The name of the parameter.</param>
        /// <param name="value">The parameter value.</param>
        protected override void ParseParameter(string paramName, string value)
        {
            switch (paramName)
            {
                case "kind":
                    ParseKind(value);
                    break;
                case "name":
                    break;
                case "value":
                    Model.Value = value;
                    break;
                case "defaultValue":
                    Model.DefaultValue = value;
                    break;
                case "values":
                    Model.Values = value;
                    break;
                case "enabled":
                    Model.Enabled = ParseBoolOrThrow(value, "Enabled");
                    break;
                case "visibleOnlyThroughRef":
                    Model.VisibleOnlyThroughRef = ParseBoolOrThrow(value, "VisibleOnlyThroughRef");
                    break;
                case "visible":
                    Model.Visible = ParseBoolOrThrow(value, "Visible");
                    break;
                case "readOnly":
                    Model.ReadOnly = ParseBoolOrThrow(value, "ReadOnly");
                    break;
                case "readOnlyValue":
                    Model.ReadOnlyValue = value;
                    break;
                case "fieldName":
                    Model.FieldName = value;
                    break;
                case "noField":
                    Model.NoField = ParseBoolOrThrow(value, paramName);
                    break;
                case "staticConfAction":
                {
                    if (Enum.TryParse(value, out ConfigurationAction action))
                    {
                        Model.StaticConfigurationAction = action;
                    }
                    else
                    {
                        throw new RTAttributeException(string.Format("Could not parse ConfigurationAction from '{1}'. Valid values are: {0}.",
                                                                     string.Join(", ", Enum.GetNames(typeof(ConfigurationAction))),
                                                                     value));
                    }
                    break;
                }
                default:
                    throw new RTAttributeException($"Unknown Property Attribute parameter: \"{paramName}\".");
            }
        }

        private void ParseKind(string value)
        {
            switch (value)
            {
                case "Get":
                    Model.IsPropertyGetter = true;
                    break;
                case "Set":
                    Model.IsPropertySetter = true;
                    break;
                case "Enum":
                    Model.IsEnum = true;
                    break;
                case "Proc":
                    Model.Type = CoreType.Procedure;
                    break;
                case "None":
                    Model.Skip = true;
                    break;
                default:
                    throw new RTAttributeException($"Unknown property kind: {value}.");
            }
        }
    }
}
