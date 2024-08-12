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


namespace openDAQDemoNet;


/// <summary>
/// Class describing items from the device scan (<c>_instance.AvailableDevices</c>).
/// </summary>
public class ChildDevice
{
    private bool _isUsed;

    /// <summary>
    /// Initializes a new instance of the <see cref="ChildDevice"/> class.
    /// </summary>
    /// <param name="used">If set to <c>true</c>, the device is already in use; otherwise <c>false</c>.</param>
    /// <param name="name">The device name.</param>
    /// <param name="connectionString">The connection string.</param>
    public ChildDevice(bool used, string name, string connectionString)
    {
        _isUsed = used;

        this.Name = name;
        this.ConnectionString = connectionString;
    }

    #region fields to show in table

    /// <summary>
    /// Gets a value indicating whether this <see cref="ChildDevice"/> is used.
    /// </summary>
    /// <value>
    ///   <c>yes</c> if used; otherwise, <c>no</c>.
    /// </value>
    [DisplayName("Used")]
    public string Used => _isUsed ? "yes" : "no";

    /// <summary>
    /// Gets the device name.
    /// </summary>
    [DisplayName("Name")]
    public string Name { get; private set; }

    /// <summary>
    /// Gets the connection string.
    /// </summary>
    [DisplayName("Connection string")]
    public string ConnectionString { get; private set; }

    #endregion

    /// <summary>
    /// Sets the used state (private property).
    /// </summary>
    internal void SetUsed() => _isUsed = true;

    /// <summary>
    /// Gets the used state (private property).
    /// </summary>
    internal bool IsUsed => _isUsed;
}
