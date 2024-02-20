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


/// <summary>
/// This interface is intended to simplify the usage of class <see cref="ListObject{TValue}"/>.
/// </summary>
/// <remarks>
/// With this interface the developer has the possibility to use directly the familiar
/// <see cref="IList{T}"/> and <see cref="IDisposable"/> interfaces of .NET,
/// instead of the OpenDAQ class <see cref="ListObject{TValue}"/>.
/// </remarks>
public interface IListObject<TValue> : IList<TValue>, IDisposable
    where TValue : BaseObject
{
    /// <summary>
    /// Gets a value indicating whether this instance has been disposed of.
    /// </summary>
    bool IsDisposed();
}
