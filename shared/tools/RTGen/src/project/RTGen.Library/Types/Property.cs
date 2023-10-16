using System;
using System.Reflection;
using RTGen.Interfaces;
using RTGen.Util;

namespace RTGen.Types
{
    /// <summary>RT Core property info.</summary>
    public class Property : IProperty
    {
        private const string PROPERTY_VARIABLE_NAME_SUFFIX = "Prop";
        private static readonly Type PropertyType = typeof(IProperty);
        private static readonly Type BoolType = typeof(bool);

        private bool? _isPropertySetter;
        private bool? _isPropertyGetter;
        private CoreType? _type;

        /// <summary>Initializes an instance of the object.</summary>
        public Property()
        {
            Visible = true;
        }

        /// <summary>Initializes an instance of the object with the specified method</summary>
        /// <param name="method">The method that the property wraps.</param>
        /// <param name="overrides"></param>
        public Property(IMethod method, IProperty overrides) : this()
        {
            Method = method;

            GetPropName();
            SetPropType();

            Visible = !(Type == CoreType.Function || Type == CoreType.Procedure);

            if (overrides != null)
            {
                ApplyOverrides(overrides);
            }
        }

        public Property(string name, CoreType type, ITypeName typeName, string defaultValue)
        {
            Name = name;

            Type = type;
            TypeName = typeName;
            DefaultValue = defaultValue;
            IsPropertyGetter = true;

            Visible = !(Type == CoreType.Function || Type == CoreType.Procedure);

            VariableName = Name.Capitalize() + PROPERTY_VARIABLE_NAME_SUFFIX;
            FieldName = Name.Uncapitalize();
        }

        /// <summary>Creates a new enum property.</summary>
        /// <param name="name">The name of the property.</param>
        /// <param name="typeName">The type of the enum.</param>
        /// <param name="defaultValue">The starting / default value.</param>
        /// <param name="values">The list of enum values.</param>
        /// <returns>A new Property instance representing an enum property.</returns>
        public static Property EnumProperty(string name, string defaultValue, string values, ITypeName typeName)
        {
            return new Property(name, CoreType.Int, typeName, defaultValue)
            {
                IsEnum = true,
                IsEnumType = typeName != null,
                Values = values,
            };
        }

        private ITypeName TypeName { get; set; }

        // IProperty

        /// <summary>The name or key of the property.</summary>
        public string Name { get; private set; }

        /// <summary>Registered property type.</summary>
        public CoreType? Type
        {
            get => _type;
            set
            {
                _type = value;

                if (_type == CoreType.Procedure || _type == CoreType.Function)
                {
                    if (string.IsNullOrEmpty(DefaultValue))
                    {
                        DefaultValue = "nullptr";
                    }

                    IsPropertyGetter = false;

                    if (Method != null)
                    {
                        UpdatePropertyName();
                    }

                    Visible = false;
                }
            }
        }

        /// <summary>Whether the property value is an enumeration field.</summary>
        public bool? IsEnum { get; set; }

        /// <summary>Default property value.</summary>
        public string DefaultValue { get; set; }

        /// <summary>Value to set in object constructor.</summary>
        public string Value { get; set; }

        /// <summary>Value when set to read-only.</summary>
        public string ReadOnlyValue { get; set; }

        /// <summary>Is the property accessible only through EvalValue.</summary>
        public bool? VisibleOnlyThroughRef { get; set; }

        /// <summary>Whether the property is visible on enumeration.</summary>
        public bool? Visible { get; set; }

        /// <summary>Can property-value change.</summary>
        public bool? ReadOnly { get; set; }

        /// <summary>Whether the property is enabled.</summary>
        public bool? Enabled { get; set; }

        /// <summary>PropertyInfo values list.</summary>
        public string Values { get; set; }

        /// <summary>Is the interface method a setter.</summary>
        public bool? IsPropertySetter
        {
            get => _isPropertySetter;
            set
            {
                _isPropertySetter = value;
                UpdateProperty();
            }
        }

        /// <summary>Is the interface method a getter.</summary>
        public bool? IsPropertyGetter
        {
            get => _isPropertyGetter;
            set
            {
                _isPropertyGetter = value;
                UpdateProperty();
            }
        }

        /// <summary>Generated field variable name.</summary>
        public string FieldName { get; set; }

        /// <summary>The generated helper field name.</summary>
        public string VariableName { get; set; }

        public IMethod Method { get; }

        /// <summary>Is this property value and enumeration key.</summary>
        public bool IsEnumType { get; set; }

        /// <summary>Configuration action.</summary>
        public ConfigurationAction? StaticConfigurationAction { get; set; }

        /// <summary>Do not generate a field wrapping the property.</summary>
        public bool NoField { get; set; }

        /// <summary>Do not generate anything as this is not a property.</summary>
        public bool Skip { get; set; }

        public void ApplyOverrides(IProperty property)
        {
            foreach (PropertyInfo propertyInfo in PropertyType.GetProperties())
            {
                if (!propertyInfo.CanWrite)
                {
                    continue;
                }

                object thisValue = propertyInfo.GetValue(this, null);
                object overrideValue = propertyInfo.GetValue(property, null);
                object defaultValue = propertyInfo.PropertyType.GetInitialDefaultValue();

                if (overrideValue == null)
                {
                    continue;
                }

                if (thisValue == null)
                {
                    propertyInfo.SetValue(this, overrideValue);
                }
                else if (!thisValue.Equals(overrideValue) && (!overrideValue.Equals(defaultValue)))
                {
                    propertyInfo.SetValue(this, overrideValue);
                }
            }
        }

        private void UpdateProperty()
        {
            if (Method != null)
            {
                SetPropType();
            }

            if (IsPropertySetter.GetValueOrDefault() || IsPropertyGetter.GetValueOrDefault())
            {
                Name = Name.Capitalize();
            }
            else if (Method != null)
            {
                Name = Method.Name;
            }
        }

        private static bool CheckIsPropertySetter(IMethod method)
        {
            if (method.Name.StartsWith("set", StringComparison.Ordinal) &&
                method.Arguments.Count == 1 &&
                !method.Arguments[0].IsOutParam
            )
            {
                return true;
            }

            return false;
        }

        private void GetPropName()
        {
            SetIsPropertyGetter();
            if (CheckIsPropertySetter(Method))
            {
                _isPropertySetter = true;
            }

            UpdatePropertyName();
        }

        private void UpdatePropertyName()
        {
            Name = IsPropertyGetter.GetValueOrDefault()
                       ? Method.Name.Substring(3, Method.Name.Length - 3)
                       : Method.Name;

            VariableName = Name.Capitalize() + PROPERTY_VARIABLE_NAME_SUFFIX;
            FieldName = Name.Uncapitalize();
        }

        private void SetIsPropertyGetter()
        {
            if (Type != CoreType.Procedure &&
                Method.Name.StartsWith("get", StringComparison.Ordinal) &&
                Method.Arguments.Count == 1 &&
                Method.Arguments[0].IsOutParam
            )
            {
                _isPropertyGetter = true;
            }
            else
            {
                _isPropertyGetter = false;
            }
        }

        private void SetPropType()
        {
            CoreType propType;

            string defaultValue;

            if (IsPropertyGetter.GetValueOrDefault() || IsPropertySetter.GetValueOrDefault())
            {
                ITypeName argType = Method.Arguments[0].Type;

                if (IsPropertyGetter.GetValueOrDefault())
                {
                    TypeName = argType;
                }

                switch (argType.Name)
                {
                    case "Bool":
                        propType = CoreType.Bool;
                        defaultValue = "false";
                        break;
                    case "Int":
                        propType = CoreType.Int;
                        defaultValue = "0";
                        break;
                    case "Float":
                        propType = CoreType.Float;
                        defaultValue = "0";
                        break;
                    case "IList":
                        propType = CoreType.List;
                        defaultValue = "nullptr";
                        break;
                    case "IDict":
                        propType = CoreType.Dictionary;
                        defaultValue = "nullptr";
                        break;
                    case "IRatio":
                        propType = CoreType.Ratio;
                        defaultValue = "nullptr";
                        break;
                    case "IString":
                        propType = CoreType.String;
                        defaultValue = "\"\"";
                        break;
                    case "IBinaryData":
                        propType = CoreType.BinaryData;
                        defaultValue = "nullptr";
                        break;
                    case "IIterable":
                        propType = CoreType.Iterable;
                        defaultValue = "nullptr";
                        break;
                    default:
                        if (argType.Flags.IsValueType)
                        {
                            propType = CoreType.Int;
                            defaultValue = "0";
                            IsEnumType = true;
                        }
                        else
                        {
                            propType = CoreType.Object;
                            defaultValue = "nullptr";
                        }

                        break;
                }
            }
            else
            {
                propType = CoreType.Procedure;
                defaultValue = "nullptr";
            }

            if (string.IsNullOrEmpty(DefaultValue))
            {
                DefaultValue = defaultValue;
            }

            _type = propType;
        }

        public string GetFieldInitialization()
        {
            if (!IsPropertyGetter.GetValueOrDefault())
            {
                return null;
            }

            string format = string.IsNullOrEmpty(Value)
                                ? "{0}(this, {1})"
                                : "{0}(this, {1}, {2})";

            switch (Type)
            {
                case CoreType.Int:
                case CoreType.Bool:
                case CoreType.List:
                case CoreType.Ratio:
                case CoreType.Float:
                case CoreType.String:
                case CoreType.Object:
                case CoreType.Dictionary:
                    return string.Format(format, FieldName, VariableName, Value);
            }

            return null;
        }

        public string GetImplementationField()
        {
            if (!IsPropertyGetter.GetValueOrDefault())
            {
                return null;
            }

            const string VALUE_TEMPLATE = "    PropertyValue<{0}> {1};";

            switch (Type)
            {
                case CoreType.Int:
                    string typeName = IsEnumType ? TypeName.ReturnType() : Type.ToString();
                    return string.Format(VALUE_TEMPLATE, typeName, FieldName);
                case CoreType.Bool:
                case CoreType.Float:
                    return string.Format(VALUE_TEMPLATE, Type.ToString(), FieldName);
                case CoreType.Ratio:
                    return string.Format(VALUE_TEMPLATE, "IRatio", FieldName);
                case CoreType.String:
                    return string.Format(VALUE_TEMPLATE, "IString", FieldName);
                case CoreType.List:
                case CoreType.Dictionary:
                case CoreType.Object:
                    return string.Format(VALUE_TEMPLATE, TypeName.Name, FieldName);
                case CoreType.Undefined:
                    Log.Warning($"Unknown property type for property \"{Name}\"");
                    return null;
                case CoreType.Function:
                case CoreType.Procedure:
                case CoreType.BinaryData:
                    return null;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        /// <summary>Creates a new object that is a deep copy of the current instance. </summary>
        /// <returns>A new object that is a deep copy of this instance.</returns>
        public IProperty Clone()
        {
            return new Property(Method, null)
            {
                _isPropertyGetter = this._isPropertyGetter,
                _isPropertySetter = this.IsPropertySetter,
                _type = this._type,
                Name = this.Name,
                TypeName = this.TypeName.Clone(),
                IsEnum = this.IsEnum,
                DefaultValue = this.DefaultValue,
                Value = this.Value,
                ReadOnlyValue = this.ReadOnlyValue,
                VisibleOnlyThroughRef = this.VisibleOnlyThroughRef,
                Visible = this.Visible,
                ReadOnly = this.ReadOnly,
                Enabled = this.Enabled,
                Values = this.Values,
                FieldName = this.FieldName,
                VariableName = this.VariableName,
                IsEnumType = this.IsEnumType,
                StaticConfigurationAction = this.StaticConfigurationAction,
                NoField = this.NoField
            };
        }
    }
}
