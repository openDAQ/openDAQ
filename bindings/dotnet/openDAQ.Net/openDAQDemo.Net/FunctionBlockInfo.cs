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

using Daq.Core.OpenDAQ;


namespace openDAQDemoNet;


/// <summary>
/// Class describing available function blocks.
/// </summary>
public class FunctionBlockInfo
{
    private readonly FunctionBlockType _functionBlockType;

    /// <summary>
    /// Initializes a new instance of the <see cref="FunctionBlockInfo"/> class.
    /// </summary>
    /// <param name="name">The function-block-type object.</param>
    public FunctionBlockInfo(FunctionBlockType functionBlockType)
    {
        _functionBlockType = functionBlockType;
    }

    #region fields to show in table

    /// <summary>
    /// Gets the unique function-block-type ID.
    /// </summary>
    [DisplayName("Type ID")]
    public string Id => _functionBlockType.Id;

    /// <summary>
    /// Gets the user-friendly function-block-type name.
    /// </summary>
    [DisplayName("Name")]
    public string Name => _functionBlockType.Name;

    /// <summary>
    /// Gets the function-block-type description.
    /// </summary>
    [DisplayName("Description")]
    public string Description => _functionBlockType.Description;

    #endregion
}
