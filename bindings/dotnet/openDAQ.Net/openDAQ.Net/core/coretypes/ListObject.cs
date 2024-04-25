/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
//     RTGen (CSharpGenerator v1.0.0) on 29.04.2024 15:45:53.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawListObject : RawBaseObject
{
    //ErrorCode getItemAt(daq.SizeT index, daq.IBaseObject** obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, nuint, out IntPtr, ErrorCode> GetItemAt;
    //ErrorCode getCount(daq.SizeT* size); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out nuint, ErrorCode> GetCount;
    //ErrorCode setItemAt(daq.SizeT index, daq.IBaseObject* obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, nuint, IntPtr, ErrorCode> SetItemAt;
    //ErrorCode pushBack(daq.IBaseObject* obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> PushBack;
    //ErrorCode pushFront(daq.IBaseObject* obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> PushFront;
    //ErrorCode moveBack(daq.IBaseObject* obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> MoveBack;
    //ErrorCode moveFront(daq.IBaseObject* obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> MoveFront;
    //ErrorCode popBack(daq.IBaseObject** obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> PopBack;
    //ErrorCode popFront(daq.IBaseObject** obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> PopFront;
    //ErrorCode insertAt(daq.SizeT index, daq.IBaseObject* obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, nuint, IntPtr, ErrorCode> InsertAt;
    //ErrorCode removeAt(daq.SizeT index, daq.IBaseObject** obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, nuint, out IntPtr, ErrorCode> RemoveAt;
    //ErrorCode deleteAt(daq.SizeT index); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, nuint, ErrorCode> DeleteAt;
    //ErrorCode clear(); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, ErrorCode> Clear;
    //ErrorCode createStartIterator(daq.IIterator** iterator); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> CreateStartIterator;
    //ErrorCode createEndIterator(daq.IIterator** iterator); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> CreateEndIterator;
}

/// <summary>Represents a heterogeneous collection of objects that can be individually accessed by index.</summary>
[Guid("e7866bcc-0563-5504-b61b-a8116b614d8f")]
public class ListObject<TValue> : BaseObject, IListObject<TValue>
    where TValue : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawListObject _rawListObject;

    internal ListObject(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawListObject = Marshal.PtrToStructure<RawListObject>(objVirtualTable);
    }

    /// <summary>Gets the number of elements contained in the list.</summary>
    public nuint Count
    {
        get
        {
            //native output argument
            nuint size;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawListObject.GetCount(base.NativePointer, out size);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return size;
        }
    }
    /// <summary>Gets the element at a specific position.</summary>
    /// <remarks>
    /// The reference count of the element that is retrieved is incremented. The client is
    /// responsible for calling <c>releaseRef</c> when the element is no longer needed.
    /// </remarks>
    /// <param name="index">The zero-based index of the element to get.</param>
    /// <returns>The element at the specified index.</returns>
    public TValue GetItemAt(nuint index)
    {
        //native output argument
        IntPtr objPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawListObject.GetItemAt(base.NativePointer, index, out objPtr);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return BaseObject.CreateInstance<TValue>(objPtr, incrementReference: false);
    }

    /// <summary>Sets the element at a specific position.</summary>
    /// <remarks>The reference count of the element is incremented.</remarks>
    /// <param name="index">The zero-based index of the element to set.</param>
    /// <param name="obj">The element to set at the specified index.</param>
    public void SetItemAt(nuint index, TValue obj)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawListObject.SetItemAt(base.NativePointer, index, obj.NativePointer);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Inserts the element at the end of the list.</summary>
    /// <remarks>The reference count of the element is incremented.</remarks>
    /// <param name="obj">The element to insert.</param>
    public void PushBack(TValue obj)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawListObject.PushBack(base.NativePointer, obj.NativePointer);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Inserts the element at the start of the list.</summary>
    /// <remarks>The reference count of the element is incremented.</remarks>
    /// <param name="obj">The element to insert.</param>
    public void PushFront(TValue obj)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawListObject.PushFront(base.NativePointer, obj.NativePointer);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Inserts the element at the end of the list without incrementing the reference count.</summary>
    /// <remarks>
    /// The reference count of the element is not incremented. The client can use this method when it no
    /// longer needs to access the element after calling the method.
    /// </remarks>
    /// <param name="obj">The element to insert.</param>
    public void MoveBack(TValue obj)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawListObject.MoveBack(base.NativePointer, obj.NativePointer);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Inserts the element at the start of the list without incrementing the reference count.</summary>
    /// <remarks>
    /// The reference count of the element is not incremented. The client can use this method when it no
    /// longer needs to access the element after calling the method.
    /// </remarks>
    /// <param name="obj">The element to insert.</param>
    public void MoveFront(TValue obj)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawListObject.MoveFront(base.NativePointer, obj.NativePointer);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Gets the element from the end of the list.</summary>
    /// <remarks>
    /// The reference count of the element that is retrieved is incremented. The client is
    /// responsible for calling <c>releaseRef</c> when the element is no longer needed.
    /// </remarks>
    /// <returns>The extracted element.</returns>
    public TValue PopBack()
    {
        //native output argument
        IntPtr objPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawListObject.PopBack(base.NativePointer, out objPtr);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return BaseObject.CreateInstance<TValue>(objPtr, incrementReference: false);
    }

    /// <summary>Gets the element from the start of the list.</summary>
    /// <remarks>
    /// The reference count of the element that is retrieved is incremented. The client is
    /// responsible for calling <c>releaseRef</c> when the element is no longer needed.
    /// </remarks>
    /// <returns>The extracted element.</returns>
    public TValue PopFront()
    {
        //native output argument
        IntPtr objPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawListObject.PopFront(base.NativePointer, out objPtr);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return BaseObject.CreateInstance<TValue>(objPtr, incrementReference: false);
    }

    /// <summary>Inserts the element at a specific position.</summary>
    /// <remarks>The reference count of the element is incremented.</remarks>
    /// <param name="index">The zero-based index of the element to insert.</param>
    /// <param name="obj">The element to insert at the specified index.</param>
    public void InsertAt(nuint index, TValue obj)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawListObject.InsertAt(base.NativePointer, index, obj.NativePointer);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Removes the element at a specific position.</summary>
    /// <remarks>
    /// The client is responsible for calling <c>releaseRef</c> when the element is no longer needed.
    /// If the client does not need the element after it is removed, it should call <c>delete</c> method.
    /// </remarks>
    /// <param name="index">The zero-based index of the element to remove.</param>
    /// <returns>The removed element.</returns>
    public TValue RemoveAt(nuint index)
    {
        //native output argument
        IntPtr objPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawListObject.RemoveAt(base.NativePointer, index, out objPtr);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return BaseObject.CreateInstance<TValue>(objPtr, incrementReference: false);
    }

    /// <summary>Deletes the element at a specific position.</summary>
    /// <remarks>If the client needs the element deleted, it should use <c>removeAt</c> method.</remarks>
    /// <param name="index">The zero-based index of the element to remove.</param>
    public void DeleteAt(nuint index)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawListObject.DeleteAt(base.NativePointer, index);

            if (Daq.Core.Types.Result.Failed(errorCode))
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
            ErrorCode errorCode = (ErrorCode)_rawListObject.Clear(base.NativePointer);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Creates and returns the start iterator of the list.</summary>
    /// <remarks>Use iterators to iterate through the elements.</remarks>
    /// <returns>The start iterator.</returns>
    public IEnumerator<TValue> CreateStartIterator()
    {
        //native output argument
        IntPtr iteratorPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawListObject.CreateStartIterator(base.NativePointer, out iteratorPtr);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return new Iterator<TValue>(iteratorPtr, incrementReference: false);
    }

    /// <summary>Creates and returns the stop iterator of the list.</summary>
    /// <remarks>Use iterators to iterate through the elements.</remarks>
    /// <returns>The stop iterator.</returns>
    public IEnumerator<TValue> CreateEndIterator()
    {
        //native output argument
        IntPtr iteratorPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawListObject.CreateEndIterator(base.NativePointer, out iteratorPtr);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return new Iterator<TValue>(iteratorPtr, incrementReference: false);
    }

    #region IListObject<TValue> implementation

    /// <inheritdoc/>
    bool IListObject<TValue>.IsDisposed()
    {
        return this.IsDisposed;
    }

    #endregion IListObject<TValue> implementation

    #region IList<TValue> implementation

    /// <inheritdoc/>
    TValue IList<TValue>.this[int index]
    {
        get => this.GetItemAt((nuint)index);
        set => this.SetItemAt((nuint)index, value);
    }

    /// <inheritdoc/>
    int IList<TValue>.IndexOf(TValue item)
    {
        int index = -1;

        // iterate through all elements
        using var iterator = this.CreateStartIterator();

        while (iterator.MoveNext())
        {
            index++;

            // return the index when 'item' found
            using TValue current = iterator.Current;
            if (current.Equals(item))
            {
                return index;
            }
        }

        return -1;
    }

    /// <inheritdoc/>
    void IList<TValue>.Insert(int index, TValue item)
    {
        this.InsertAt((nuint)index, item);
    }

    /// <inheritdoc/>
    void IList<TValue>.RemoveAt(int index)
    {
        this.DeleteAt((nuint)index); //delete not remove (deleted when reference count goes to 0)
    }

    #endregion IList<TValue> implementation

    #region ICollection<TValue> implementation

    /// <inheritdoc/>
    int ICollection<TValue>.Count => (int)this.Count;

    /// <inheritdoc/>
    bool ICollection<TValue>.IsReadOnly => false;

    /// <inheritdoc/>
    void ICollection<TValue>.Add(TValue item)
    {
        this.PushBack(item);
    }

    /// <inheritdoc/>
    void ICollection<TValue>.Clear()
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawListObject.Clear(base.NativePointer);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <inheritdoc/>
    bool ICollection<TValue>.Contains(TValue item)
    {
        return ((IList<TValue>)this).IndexOf(item) >= 0;
    }

    /// <inheritdoc/>
    void ICollection<TValue>.CopyTo(TValue[] array, int arrayIndex)
    {
        //no checks to be made since .NET will throw the appropriate exceptions
        foreach (TValue item in this)
        {
            array[arrayIndex++] = item;
        }
    }

    /// <inheritdoc/>
    bool ICollection<TValue>.Remove(TValue item)
    {
        int index = ((IList<TValue>)this).IndexOf(item);

        if (index < 0)
        {
            return false;
        }

        ((IList<TValue>)this).RemoveAt(index);

        return true;
    }

    #endregion ICollection<TValue> implementation

    #region IEnumerable<TValue> implementation

    /// <inheritdoc/>
    IEnumerator<TValue> IEnumerable<TValue>.GetEnumerator()
    {
        return this.CreateStartIterator();
    }

    #endregion IEnumerable<TValue> implementation

    #region IEnumerable implementation

    /// <inheritdoc/>
    IEnumerator IEnumerable.GetEnumerator()
    {
        return ((IEnumerable<TValue>)this).GetEnumerator();
    }

    #endregion IEnumerable implementation
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
    //ErrorCode createList(daq.IList** obj); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createList(out IntPtr obj);

    public static ErrorCode CreateList<TValue>(out IListObject<TValue> obj)
        where TValue : BaseObject
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createList(out objPtr);

        if (Daq.Core.Types.Result.Succeeded(errorCode))
        {
            //create object
            obj = new ListObject<TValue>(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static IListObject<TValue> CreateList<TValue>()
        where TValue : BaseObject
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createList(out objPtr);

        if (Daq.Core.Types.Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new ListObject<TValue>(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
