/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


using Daq.Core.Types;


namespace Daq.Core.Objects;


// Property-factory functions of the &apos;CoreObjects&apos; library.
public static partial class PropertyFactory
{
    /// <summary>
    /// Creates a boolean Property object with a default value and optional Visible state.
    /// </summary>
    /// <remarks>The Property Value type is `ctBool`.</remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="defaultValue">The boolean default value.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property BoolProperty(string name,
                                        bool defaultValue,
                                        bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateBoolProperty(name, defaultValue, visible);
        return obj;
    }

    /// <summary>
    /// Creates an integer Property object with a default value and optional Visible state.
    /// </summary>
    /// <remarks>The Property Value type is `ctInt`.</remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="defaultValue">The integer default value.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property IntProperty(string name,
                                       long defaultValue,
                                       bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateIntProperty(name, defaultValue, visible);
        return obj;
    }

    /// <summary>
    /// Creates a floating point value Property object with a default value and optional Visible state.
    /// </summary>
    /// <remarks>The Property Value type is `ctFloat`.</remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="defaultValue">The float default value.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property FloatProperty(string name,
                                         double defaultValue,
                                         bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateFloatProperty(name, defaultValue, visible);
        return obj;
    }

    /// <summary>
    /// Creates a string Property object with a default value and optional Visible state.
    /// </summary>
    /// <remarks>The Property Value type is `ctString`.</remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="defaultValue">The string default value.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property StringProperty(string name,
                                          string defaultValue,
                                          bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateStringProperty(name, defaultValue, visible);
        return obj;
    }

    /// <summary>
    /// Creates a list Property object with a default value and optional Visible state.
    /// </summary>
    /// <remarks>
    /// The Property Value type is `ctList`.<br/>
    /// The list passed as `defaultValue` must be homogeneous.<br/>
    /// The Property's Item type field will be set according to defaultValue list type.
    /// </remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="defaultValue">The list default value.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property ListProperty(string name,
                                        IListObject<BaseObject> defaultValue,
                                        bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateListProperty(name, defaultValue, visible);
        return obj;
    }

    /// <summary>
    /// Creates a dictionary Property object with a default value and optional Visible state.
    /// </summary>
    /// <remarks>
    /// The Property Value type is `ctDict`.<br/>
    /// The dictionary passed as default value must have homogeneous key and value lists.<br/>
    /// The Property's Item type field will be set according to defaultValue dictionary Item type. The same goes for
    /// the Key type.
    /// </remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="defaultValue">The dictionary default value.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property DictProperty(string name,
                                        IDictObject<BaseObject, BaseObject> defaultValue,
                                        bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateDictProperty(name, defaultValue, visible);
        return obj;
    }

    /// <summary>
    /// Creates a ratio Property object with a default value and optional Visible state.
    /// </summary>
    /// <remarks>The Property Value type is `ctRatio`.</remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="defaultValue">The ratio default value.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property RatioProperty(string name,
                                         Ratio defaultValue,
                                         bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateRatioProperty(name, defaultValue, visible);
        return obj;
    }

    /// <summary>
    /// Creates an object-type config object with a default value.
    /// </summary>
    /// <remarks>
    /// The Property Value type is `ctObject`.<br/>
    /// Object properties cannot be have any meta-data other than their name, description, and default value configured.<br/>
    /// The PropertyObject default value can only be a base PropertyObject type (not a descendant type).
    /// </remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="defaultValue">The Property object default value.</param>
    /// <returns>The Property object.</returns>
    public static Property ObjectProperty(string name,
                                          PropertyObject defaultValue = null)
    {
        Property obj = CoreObjectsFactory.CreateObjectProperty(name, defaultValue);
        return obj;
    }

    /// <summary>
    /// Creates a function- or procedure-type Property object. Requires the a CallableInfo object
    /// to specify the argument type/count and function return type.
    /// </summary>
    /// <remarks>
    /// The Property Value type is `ctFunction` or `ctProc`, depending on if `callableInfo` contains information
    /// on the return type or not.
    /// </remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="callableInfo">Information about the callable argument type/count and return type.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property FunctionProperty(string name,
                                            CallableInfo callableInfo,
                                            bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateFunctionProperty(name, callableInfo, visible);
        return obj;
    }

    /// <summary>
    /// Creates a Reference Property object that points at a property specified in the `referencedProperty` parameter.
    /// </summary>
    /// <param name="name">The name of the Property.</param>
    /// <param name="referencedPropertyEval">The evaluation expression that evaluates to another property.</param>
    /// <returns>The Property object.</returns>
    public static Property ReferenceProperty(string name, EvalValue referencedPropertyEval)
    {
        Property obj = CoreObjectsFactory.CreateReferenceProperty(name, referencedPropertyEval);
        return obj;
    }

    /// <summary>
    /// Creates a Selection Property object with a list of selection values. The default value
    /// is an integer index into the default selected value.
    /// </summary>
    /// <remarks>The Property Value type is `ctInt`.</remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="selectionValues">The list of selectable values.</param>
    /// <param name="defaultValue">The default index into the list of selection values.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property SelectionProperty(string name,
                                             IListObject<BaseObject> selectionValues,
                                             long defaultValue,
                                             bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateSelectionProperty(name, selectionValues, defaultValue, visible);
        return obj;
    }

    /// <summary>
    /// Creates a Selection Property object with a dictionary of selection values. The default value
    /// is an integer key into the provided dictionary.
    /// </summary>
    /// <remarks>
    /// The Property Value type is `ctInt`.<br/>
    /// The key type of the Selection values dictionary must be `ctInt`.
    /// </remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="selectionValues">The dictionary of selectable values. The key type must be `ctInt`.</param>
    /// <param name="defaultValue">The default key into the list of selection values.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property SparseSelectionProperty(string name,
                                                   IDictObject<BaseObject, BaseObject> selectionValues,
                                                   long defaultValue,
                                                   bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateSparseSelectionProperty(name, selectionValues, defaultValue, visible);
        return obj;
    }

    /// <summary>
    /// Creates a Struct Property object with a default value and its visible state.
    /// </summary>
    /// <remarks>The Property Value type is `ctStruct`.</remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="defaultValue">The default structure value.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property StructProperty(string name,
                                          Struct defaultValue,
                                          bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateStructProperty(name, defaultValue, visible);
        return obj;
    }

    /// <summary>
    /// Creates an Enumeration Property object with a default value and its visible state.
    /// </summary>
    /// <remarks>The Property Value type is `ctEnumeration`.</remarks>
    /// <param name="name">The name of the Property.</param>
    /// <param name="defaultValue">The default enumeration value.</param>
    /// <param name="visible">If true, the Property is visible.</param>
    /// <returns>The Property object.</returns>
    public static Property EnumerationProperty(string name,
                                               Enumeration defaultValue,
                                               bool visible = true)
    {
        Property obj = CoreObjectsFactory.CreateEnumerationProperty(name, defaultValue, visible);
        return obj;
    }
}
