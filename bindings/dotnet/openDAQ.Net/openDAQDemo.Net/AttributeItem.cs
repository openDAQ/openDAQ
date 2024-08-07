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
using Daq.Core.Types;

using GlblRes = global::openDAQDemoNet.Properties.Resources;


namespace openDAQDemoNet;


/// <summary>
/// Class describing <see cref="Daq.Core.OpenDAQ.Component"/> attributes.
/// </summary>
public class AttributeItem
{
    private readonly Image _editableImage = new Bitmap(16, 16); //empty image
    private readonly Image _lockedImage   = (Image)GlblRes.locked16.Clone();

    private readonly bool       _isLocked;
    private readonly string     _name;
    private readonly CoreType   _valueType;
    private readonly BaseObject _openDaqObject;

    /// <summary>
    /// Initializes a new instance of the <see cref="AttributeItem"/> class.
    /// </summary>
    /// <param name="isLocked">If set to <c>true</c>, the attribute is locked; otherwise <c>false</c>.</param>
    /// <param name="name">The attribute's name.</param>
    /// <param name="displayName">The attribute's display name.</param>
    /// <param name="value">The attribute value's text representation.</param>
    /// <param name="valueType">The attribute's value type.</param>
    /// <param name="openDaqObject">The owner of this attribute.</param>
    public AttributeItem(bool isLocked, string name, string displayName, string? value, CoreType valueType, BaseObject openDaqObject)
    {
        _isLocked    = isLocked;
        _name          = name;
        _valueType     = valueType;
        _openDaqObject = openDaqObject;

        this.DisplayName = displayName;
        this.Value       = value ?? string.Empty;
    }

    #region fields to show in table

    /// <summary>
    /// Gets an image indicating whether this <see cref="AttributeItem"/> is locked.
    /// </summary>
    [DisplayName("Locked")]
    public Image LockedImage => _isLocked ? _lockedImage : _editableImage;

    /// <summary>
    /// Gets the attribute's human readable name.
    /// </summary>
    [DisplayName("Attribute name")]
    public string DisplayName { get; }

    /// <summary>
    /// Gets the attribute value.
    /// </summary>
    [DisplayName("Value")]
    public string Value { get; }

    #endregion

    /// <summary>
    /// Gets the locked state of the attribute.
    /// </summary>
    internal bool IsLocked => _isLocked;

    /// <summary>
    /// Gets the attribute's name.
    /// </summary>
    internal string Name => _name;

    /// <summary>
    /// Gets the attribute's value type.
    /// </summary>
    internal CoreType ValueType => _valueType;

    /// <summary>
    /// Gets the openDAQ object that owns this attribute.
    /// </summary>
    internal BaseObject OpenDaqObject => _openDaqObject;
}
