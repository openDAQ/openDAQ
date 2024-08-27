/*
 * Copyright 2022-2024 openDAQ d.o.o.
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


using System.ComponentModel;
using System.ComponentModel.DataAnnotations;

using Daq.Core.Objects;
using Daq.Core.Types;

using GlblRes = global::openDAQDemoNet.Properties.Resources;


namespace openDAQDemoNet;


/// <summary>
/// Class describing property items from <see cref="Daq.Core.OpenDAQ.Component"/>s (inherits from <see cref="AttributeItem"/>).
/// </summary>
/// <seealso cref="AttributeItem" />
public class PropertyItem : AttributeItem
{
    /// <summary>
    /// Initializes a new instance of the <see cref="PropertyItem"/> class.
    /// </summary>
    /// <param name="isLocked">If set to <c>true</c>, the property is locked; otherwise <c>false</c>.</param>
    /// <param name="name">The property name.</param>
    /// <param name="value">The property value's text representation.</param>
    /// <param name="unit">The property value's unit.</param>
    /// <param name="description">The description of the property.</param>
    /// <param name="openDaqObject">The property object.</param>
    public PropertyItem(bool isLocked, string name, string? value, string unit, string description, BaseObject openDaqObject)
        : base(isLocked, name, name, value, CoreType.ctUndefined, openDaqObject)
    {
        this.Unit        = unit;
        this.Description = description;
    }

    #region fields to show in table

    //Hack: declaring base properties as `new` to display the inherited properties first in grid (DisplayAttribute.Order not working)

    /// <summary>
    /// Gets an image indicating whether this <see cref="PropertyItem"/> is locked.
    /// </summary>
    [DisplayName("Locked")]
    public new Image LockedImage => base.LockedImage;

    /// <summary>
    /// Gets the property name.
    /// </summary>
    [DisplayName("Property name")]
    public new string DisplayName => base.DisplayName; //for a `Property` the `DisplayName` always equals `Name`

    /// <summary>
    /// Gets the property value.
    /// </summary>
    [DisplayName("Value")]
    public new string Value => base.Value;

    /// <summary>
    /// Gets the property value's unit.
    /// </summary>
    [DisplayName("Unit")]
    public string Unit { get; }

    /// <summary>
    /// Gets the property description.
    /// </summary>
    [DisplayName("Description")]
    public string Description { get; }

    #endregion
}
