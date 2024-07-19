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


//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CSharpGenerator v1.0.0) on 25.06.2024 08:46:37.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawDictObject : RawBaseObject
{
    //ErrorCode get(daq.IBaseObject* key, daq.IBaseObject** value); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, out IntPtr, ErrorCode> Get;
    //ErrorCode set(daq.IBaseObject* key, daq.IBaseObject* value); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, IntPtr, ErrorCode> Set;
    //ErrorCode remove(daq.IBaseObject* key, daq.IBaseObject** value); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, out IntPtr, ErrorCode> Remove;
    //ErrorCode deleteItem(daq.IBaseObject* key); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> DeleteItem;
    //ErrorCode clear(); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, ErrorCode> Clear;
    //ErrorCode getCount(daq.SizeT* size); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out nuint, ErrorCode> GetCount;
    //ErrorCode hasKey(daq.IBaseObject* key, daq.Bool* hasKey); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, out bool, ErrorCode> HasKey;
    //ErrorCode getKeyList(daq.IList** keys); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetKeyList;
    //ErrorCode getValueList(daq.IList** values); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetValueList;
    //ErrorCode getKeys(daq.IIterable** iterable); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetKeys;
    //ErrorCode getValues(daq.IIterable** iterable); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetValues;
}

/// <summary>Represents a collection of key/value pairs.</summary>
[Guid("e3de60da-0366-5da5-8334-f9dcadff5ad0")]
public class DictObject<TKey, TValue> : BaseObject, IDictObject<TKey, TValue>
    where TKey : BaseObject
    where TValue : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawDictObject _rawDictObject;

    internal DictObject(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawDictObject = Marshal.PtrToStructure<RawDictObject>(objVirtualTable);
    }

    #region properties

    /// <summary>Gets the number of elements contained in the dictionary.</summary>
    public nuint Count
    {
        get
        {
            //native output argument
            nuint size;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawDictObject.GetCount(base.NativePointer, out size);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return size;
        }
    }

    /// <summary>Gets the list of all keys in the dictionary.</summary>
    /// <remarks>
    /// The order of the keys is not defined.
    /// <para/>
    /// The client is responsible for calling <c>releaseRef</c> when the list is no longer needed.
    /// </remarks>
    public IListObject<TKey> KeyList
    {
        get
        {
            //native output argument
            IntPtr keysPtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawDictObject.GetKeyList(base.NativePointer, out keysPtr);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (keysPtr == IntPtr.Zero)
            {
                return default;
            }

            return new ListObject<TKey>(keysPtr, incrementReference: false);
        }
    }

    /// <summary>Gets the list of all elements in the dictionary.</summary>
    /// <remarks>
    /// The order of the elements is not defined.
    /// <para/>
    /// The client is responsible for calling <c>releaseRef</c> when the list is no longer needed.
    /// </remarks>
    public IListObject<TValue> ValueList
    {
        get
        {
            //native output argument
            IntPtr valuesPtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawDictObject.GetValueList(base.NativePointer, out valuesPtr);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (valuesPtr == IntPtr.Zero)
            {
                return default;
            }

            return new ListObject<TValue>(valuesPtr, incrementReference: false);
        }
    }

    /// <summary>Gets the iterable interface of the keys.</summary>
    /// <remarks>
    /// The Iterable interface enables iteration through the keys.
    /// <para/>
    /// The client is responsible for calling <c>releaseRef</c> when the interface is no longer needed.
    /// </remarks>
    public Iterable<TKey> Keys
    {
        get
        {
            //native output argument
            IntPtr iterablePtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawDictObject.GetKeys(base.NativePointer, out iterablePtr);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (iterablePtr == IntPtr.Zero)
            {
                return default;
            }

            return new Iterable<TKey>(iterablePtr, incrementReference: false);
        }
    }

    /// <summary>Gets the iterable interface of the elements.</summary>
    /// <remarks>
    /// The Iterable interface enables iteration through the elements.
    /// <para/>
    /// The client is responsible for calling <c>releaseRef</c> when the interface is no longer needed.
    /// </remarks>
    public Iterable<TValue> Values
    {
        get
        {
            //native output argument
            IntPtr iterablePtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawDictObject.GetValues(base.NativePointer, out iterablePtr);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (iterablePtr == IntPtr.Zero)
            {
                return default;
            }

            return new Iterable<TValue>(iterablePtr, incrementReference: false);
        }
    }

    #endregion properties

    /// <summary>Gets the element with the specified key.</summary>
    /// <remarks>
    /// The reference count of the element that is retrieved is incremented. The client is
    /// responsible for calling <c>releaseRef</c> when the element is no longer needed.
    /// </remarks>
    /// <param name="key">The key of the element to get.</param>
    /// <returns>The element with the specified key.</returns>
    public TValue Get(TKey key)
    {
        //native output argument
        IntPtr valuePtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawDictObject.Get(base.NativePointer, key.NativePointer, out valuePtr);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        // validate pointer
        if (valuePtr == IntPtr.Zero)
        {
            return default;
        }

        return BaseObject.CreateInstance<TValue>(valuePtr, incrementReference: false);
    }

    /// <summary>Sets the element with the specified key.</summary>
    /// <remarks>The reference count of the key and the element is incremented.</remarks>
    /// <param name="key">The key of the element to set.</param>
    /// <param name="value">The element with the specified key.</param>
    public void Set(TKey key, TValue value)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDictObject.Set(base.NativePointer, key.NativePointer, value.NativePointer);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Removes the element with the specified key.</summary>
    /// <remarks>
    /// The client is responsible for calling <c>releaseRef</c> when the element is no longer needed.
    /// If the client does not need the element after it is removed, it should call <c>delete</c> method.
    /// </remarks>
    /// <param name="key">The key of the element to remove.</param>
    /// <returns>The element with the specified key.</returns>
    public TValue Remove(TKey key)
    {
        //native output argument
        IntPtr valuePtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawDictObject.Remove(base.NativePointer, key.NativePointer, out valuePtr);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        // validate pointer
        if (valuePtr == IntPtr.Zero)
        {
            return default;
        }

        return BaseObject.CreateInstance<TValue>(valuePtr, incrementReference: false);
    }

    /// <summary>Deletes the element with the specified key.</summary>
    /// <remarks>If the client needs the element deleted, it should use <c>removeAt</c> method.</remarks>
    /// <param name="key">The key of the element to delete.</param>
    public void DeleteItem(TKey key)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDictObject.DeleteItem(base.NativePointer, key.NativePointer);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Removes all elements from the list.</summary>
    public void Clear()
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDictObject.Clear(base.NativePointer);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Checks if the element with the specified key exists in the dictionary.</summary>
    /// <param name="key">The key of the element to check.</param>
    /// <returns>True if the element exists, False otherwise.</returns>
    public bool HasKey(TKey key)
    {
        //native output argument
        bool hasKey;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawDictObject.HasKey(base.NativePointer, key.NativePointer, out hasKey);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return hasKey;
    }

    #region IDictObject<TKey, TValue> implementation

    /// <inheritdoc/>
    bool IDictObject<TKey, TValue>.IsDisposed()
    {
        return this.IsDisposed;
    }

    #endregion IDictObject<TKey, TValue> implementation

    #region IDictionary<TKey, TValue> implementation

    /// <inheritdoc/>
    ICollection<TKey> IDictionary<TKey, TValue>.Keys => this.KeyList;

    /// <inheritdoc/>
    ICollection<TValue> IDictionary<TKey, TValue>.Values => this.ValueList;

    /// <inheritdoc/>
    TValue IDictionary<TKey, TValue>.this[TKey key]
    {
        get => this.Get(key);
        set => this.Set(key, value);
    }

    /// <inheritdoc/>
    void IDictionary<TKey, TValue>.Add(TKey key, TValue value)
    {
        this.Set(key, value);
    }

    /// <inheritdoc/>
    bool IDictionary<TKey, TValue>.ContainsKey(TKey key)
    {
        return this.HasKey(key);
    }

    /// <inheritdoc/>
    bool IDictionary<TKey, TValue>.Remove(TKey key)
    {
        try
        {
            if (this.HasKey(key))
            {
                this.DeleteItem(key);
                return true;
            }
        }
        catch //(Exception)
        {
            //intentionally left blank, we don't want an exception here and return 'false' at the end
        }

        return false;
    }

    /// <inheritdoc/>
    bool IDictionary<TKey, TValue>.TryGetValue(TKey key, [System.Diagnostics.CodeAnalysis.MaybeNullWhen(false)] out TValue value)
    {
        value = default(TValue);

        if (!this.HasKey(key))
        {
            return false;
        }

        value = ((IDictionary<TKey, TValue>)this)[key];
        return true;
    }

    #endregion IDictionary<TKey, TValue> implementation

    #region ICollection<T> implementation

    /// <inheritdoc/>
    int ICollection<KeyValuePair<TKey, TValue>>.Count => (int)this.Count;

    /// <inheritdoc/>
    bool ICollection<KeyValuePair<TKey, TValue>>.IsReadOnly => false;


    /// <inheritdoc/>
    void ICollection<KeyValuePair<TKey, TValue>>.Add(KeyValuePair<TKey, TValue> item)
    {
        this.Set(item.Key, item.Value);
    }

    /// <inheritdoc/>
    void ICollection<KeyValuePair<TKey, TValue>>.Clear()
    {
        this.Clear();
    }

    /// <inheritdoc/>
    bool ICollection<KeyValuePair<TKey, TValue>>.Contains(KeyValuePair<TKey, TValue> item)
    {
        if (!this.HasKey(item.Key))
        {
            return false;
        }

        return this.Get(item.Key).Equals(item.Value);
    }

    /// <inheritdoc/>
    void ICollection<KeyValuePair<TKey, TValue>>.CopyTo(KeyValuePair<TKey, TValue>[] array, int arrayIndex)
    {
        if (array == null)
        {
            throw new System.ArgumentNullException(nameof(array));
        }

        if ((uint)arrayIndex > (uint)array.Length)
        {
            throw new IndexOutOfRangeException(nameof(arrayIndex));
        }

        int count = (int)this.Count;

        if (array.Length - arrayIndex < count)
        {
            throw new ArgumentOutOfRangeException(nameof(arrayIndex));
        }

        foreach (TKey key in this.KeyList)
        {
            TValue value = this.Get(key);
            array[arrayIndex++] = new KeyValuePair<TKey, TValue>(key, value);
        }
    }

    /// <inheritdoc/>
    bool ICollection<KeyValuePair<TKey, TValue>>.Remove(KeyValuePair<TKey, TValue> item)
    {
        try
        {
            if (!((ICollection<KeyValuePair<TKey, TValue>>)this).Contains(item))
            {
                this.DeleteItem(item.Key);
                return true;
            }
        }
        catch //(Exception)
        {
            //intentionally left blank, we don't want an exception here and return 'false' at the end
        }

        return false;
    }

    #endregion ICollection<T> implementation

    #region IEnumerable<T> implementation

    /// <inheritdoc/>
    IEnumerator<KeyValuePair<TKey, TValue>> IEnumerable<KeyValuePair<TKey, TValue>>.GetEnumerator()
    {
        throw new NotImplementedException();
    }

    #endregion IEnumerable<T> implementation

    #region IEnumerator implementation

    /// <inheritdoc/>
    IEnumerator IEnumerable.GetEnumerator()
    {
        return ((IEnumerable<KeyValuePair<TKey, TValue>>)this).GetEnumerator();
    }

    #endregion IEnumerator implementation
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
    //ErrorCode createDict(daq.IDict** obj); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createDict(out IntPtr obj);

    public static ErrorCode CreateDict<TKey, TValue>(out IDictObject<TKey, TValue> obj)
        where TKey : BaseObject
        where TValue : BaseObject
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createDict(out objPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new DictObject<TKey, TValue>(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static IDictObject<TKey, TValue> CreateDict<TKey, TValue>()
        where TKey : BaseObject
        where TValue : BaseObject
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createDict(out objPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new DictObject<TKey, TValue>(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
