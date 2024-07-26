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

using Daq.Core.Objects;

using GlblRes = global::openDAQDemoNet.Properties.Resources;


namespace openDAQDemoNet;


/// <summary>
/// Class describing items from the device scan (<c>_instance.AvailableDevices</c>).
/// </summary>
public class PropertyItem
{
    private readonly Image _editableImage = new Bitmap(16, 16);
    private readonly Image _lockedImage = (Image)GlblRes.locked.Clone();

    private bool _isReadOnly;

    /// <summary>
    /// Initializes a new instance of the <see cref="PropertyItem"/> class.
    /// </summary>
    /// <param name="readOnly">If set to <c>true</c>, the property is read-only; otherwise <c>false</c>.</param>
    /// <param name="name">The property name.</param>
    /// <param name="value">The property value's text representation.</param>
    /// <param name="unit">The property value's unit.</param>
    public PropertyItem(bool readOnly, string name, string value, string unit)
    {
        _isReadOnly = readOnly;

        this.Name     = name;
        this.Value    = value;
        this.Unit     = unit;
    }

    #region fields to show in table

    /// <summary>
    /// Gets an image indicating whether this <see cref="PropertyItem"/> is read-only.
    /// </summary>
    [DisplayName("Read-only")]
    public Image ReadOnlyImage => _isReadOnly ? _lockedImage : _editableImage;

    /// <summary>
    /// Gets the property name.
    /// </summary>
    [DisplayName("Property name")]
    public string Name { get; }

    /// <summary>
    /// Gets the property value.
    /// </summary>
    [DisplayName("Value")]
    public string Value { get; }

    /// <summary>
    /// Gets the property value's unit.
    /// </summary>
    [DisplayName("Unit")]
    public string Unit { get; }

    #endregion

    /// <summary>
    /// Gets the read-only state (private property).
    /// </summary>
    internal bool IsReadOnly => _isReadOnly;
}
