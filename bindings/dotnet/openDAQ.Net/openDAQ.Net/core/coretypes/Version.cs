/*
 * Copyright 2022-2023 Blueberry d.o.o.
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


namespace Daq.Core.Types;


public class OpenDaqVersion
{
    public OpenDaqVersion(uint major, uint minor, uint revision)
    {
        this.Major    = major;
        this.Minor    = minor;
        this.Revision = revision;
    }

    public uint Major    { get; }
    public uint Minor    { get; }
    public uint Revision { get; }

    #region operators

    //implicit cast operators

    /// <summary>Performs an implicit conversion from <see cref="Version"/> to <see cref="Daq.Core.Types.OpenDaqVersion"/>.</summary>
    /// <param name="value">The managed <c>Version</c> value.</param>
    /// <returns>The SDK <c>OpenDaqVersion</c>.</returns>
    public static implicit operator OpenDaqVersion(Version value) => new OpenDaqVersion((uint)value.Major, (uint)value.Minor, (uint)value.Build);

    /// <summary>Performs an implicit conversion from <see cref="Daq.Core.Types.OpenDaqVersion"/> to <see cref="Version"/>.</summary>
    /// <param name="value">The SDK <c>OpenDaqVersion</c>.</param>
    /// <returns>The managed <c>Version</c> object.</returns>
    public static implicit operator Version(OpenDaqVersion value) => new Version((int)value.Major, (int)value.Minor, (int)value.Revision);

    #endregion operators

    /// <inheritdoc/>
    public override string ToString()
    {
        return $"{this.Major}.{this.Minor}.{this.Revision}";
    }
}


#region Class Factory
#endregion Class Factory
