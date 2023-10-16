using System;
using System.Runtime.Serialization;

namespace RTGen.Interfaces
{
    /// <summary>Property action on configuration step.</summary>
    [Flags]
    public enum ConfigurationAction
    {
        ///<summary>Use the mode's default action</summary> 
        Default,
        ///<summary>Do nothing</summary> 
        Skip,
        ///<summary>Update properties</summary> 
        UpdateProperties,
        ///<summary>Create from factory and update properties</summary> 
        CreateUpdate,
        ///<summary>Create from callback and update properties</summary> 
        CallbackUpdate,
        ///<summary>Clear value</summary> 
        Clear,
        ///<summary>Call IUpdatable.update()</summary> 
        Custom,
        ///<summary>Check object for appropriate action</summary> 
        CheckObject
    };


    /// <summary>RT Core property info.</summary>
    public interface IProperty : ICloneable<IProperty>
    {
        /// <summary>The property name.</summary>
        string Name { get; }

        /// <summary>The property core type.</summary>
        CoreType? Type { get; set; }

        /// <summary>Whether the property value is an enumeration field.</summary>
        bool? IsEnum { get; set; }

        /// <summary>Whether the property is enabled.</summary>
        bool? Enabled { get; set; }

        /// <summary>Default property value.</summary>
        string DefaultValue { get; set; }

        /// <summary>Value to set in object constructor.</summary>
        string Value { get; set; }

        /// <summary>PropertyInfo values list.</summary>
        string Values { get; set; }

        /// <summary>Value when set to read-only.</summary>
        string ReadOnlyValue { get; set; }

        /// <summary>Is the property accessible only through EvalValue.</summary>
        bool? VisibleOnlyThroughRef { get; set; }

        /// <summary>Whether the property is visible on enumeration.</summary>
        bool? Visible { get; set; }

        /// <summary>Can property-value change.</summary>
        bool? ReadOnly { get; set; }

        /// <summary>The generated helper field name.</summary>
        string VariableName { get; set; }

        /// <summary>Generated field variable name.</summary>
        string FieldName { get; set; }

        /// <summary>Configuration action.</summary>
        ConfigurationAction? StaticConfigurationAction { get; set; }

        [IgnoreDataMember]
        IMethod Method { get; }

        /// <summary>Is the interface method a setter.</summary>
        bool? IsPropertySetter { get; set; }

        /// <summary>Is the interface method a getter.</summary>
        bool? IsPropertyGetter { get; set; }

        string GetFieldInitialization();

        string GetImplementationField();

        /// <summary>Do not generate a field wrapping the property.</summary>
        bool NoField { get; set; }

        /// <summary>Do not generate anything as this is not a property.</summary>
        bool Skip { get; set; }
    }
}
