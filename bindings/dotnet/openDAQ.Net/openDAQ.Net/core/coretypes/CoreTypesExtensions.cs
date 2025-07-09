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



namespace Daq.Core.Types;


/// <summary>Extension functions for objects in the &apos;CoreTypes&apos; library.</summary>
public static partial class CoreTypesExtensions
{
    #region ListObject

    /// <summary>
    /// Extension that casts a list of <c>&lt;BaseObject&gt;</c> to a list of <c>&lt;<typeparamref name="TValue"/>&gt;</c>.
    /// </summary>
    /// <remarks>Usage:<br/><c>var floatObjectList = baseObjectList.CastList&lt;FloatObject&gt;();</c></remarks>
    /// <typeparam name="TValue">The target type of the list items.</typeparam>
    /// <param name="listObj">The list object.</param>
    /// <returns>The type-cast <c>IListObject&lt;TValue&gt;</c> interface of the list.</returns>
    public static IListObject<TValue> CastList<TValue>(this IListObject<BaseObject> listObj)
        where TValue : BaseObject
    {
        return ((ListObject<BaseObject>)listObj).CastList<TValue>();
    }

    /// <summary>
    /// Extension that casts a list of <c>&lt;<typeparamref name="TValue"/>&gt;</c> to a list of <c>&lt;BaseObject&gt;</c>.
    /// </summary>
    /// <remarks>Usage:<br/><c>var baseObjectList = floatObjectList.CastList();</c></remarks>
    /// <typeparam name="TValue">The source type of the list items.</typeparam>
    /// <param name="listObj">The list object.</param>
    /// <returns>The type-cast <c>IListObject&lt;BaseObject&gt;</c> interface of the list.</returns>
    public static IListObject<BaseObject> CastList<TValue>(this IListObject<TValue> listObj)
        where TValue : BaseObject
    {
        return ((ListObject<TValue>)listObj).CastList<BaseObject>();
    }

    #endregion ListObject

    #region DictObject

    /// <summary>
    /// Extension that casts a dictionary of <c>&lt;BaseObject,BaseObject&gt;</c> to a dictionary of <c>&lt;<typeparamref name="TKey"/>,<typeparamref name="TValue"/>&gt;</c>.
    /// </summary>
    /// <remarks>Usage:<br/><c>var stringObjectDict = baseObjectDict.CastDict&lt;IntegerObject, StringObject&gt;();</c></remarks>
    /// <typeparam name="TKey">The type of the dictionary item key.</typeparam>
    /// <typeparam name="TValue">The type of the dictionary item value.</typeparam>
    /// <param name="dictObj">The dictionary object.</param>
    /// <returns>The type-cast <c>IDictObject&lt;TKey,TValue&gt;</c> interface of the dictionary.</returns>
    public static IDictObject<TKey, TValue> CastDict<TKey, TValue>(this IDictObject<BaseObject, BaseObject> dictObj)
        where TKey : BaseObject
        where TValue : BaseObject
    {
        return ((DictObject<BaseObject, BaseObject>)dictObj).CastDict<TKey, TValue>();
    }

    /// <summary>
    /// Extension that casts a dictionary of <c>&lt;<typeparamref name="TKey"/>,<typeparamref name="TValue"/>&gt;</c> to a dictionary of <c>&lt;BaseObject,BaseObject&gt;</c>.
    /// </summary>
    /// <remarks>Usage:<br/><c>var baseObjectDict = stringObjectDict.CastDict();</c></remarks>
    /// <typeparam name="TKey">The type of the dictionary item key.</typeparam>
    /// <typeparam name="TValue">The type of the dictionary item value.</typeparam>
    /// <param name="dictObj">The dictionary object.</param>
    /// <returns>The type-cast <c>IDictObject&lt;BaseObject,BaseObject&gt;</c> interface of the dictionary.</returns>
    public static IDictObject<BaseObject, BaseObject> CastDict<TKey, TValue>(this IDictObject<TKey, TValue> dictObj)
        where TKey : BaseObject
        where TValue : BaseObject
    {
        return ((DictObject<TKey, TValue>)dictObj).CastDict<BaseObject, BaseObject>();
    }

    #endregion DictObject
}
