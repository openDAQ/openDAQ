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


//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CSharpGenerator v1.0.0) on 04.09.2024 17:45:21.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawIterator : RawBaseObject
{
    //ErrorCode moveNext(); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, ErrorCode> MoveNext;
    //ErrorCode getCurrent(daq.IBaseObject** obj); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetCurrent;
}

/// <summary>Use this interface to iterator through items of a container object.</summary>
/// <remarks>
/// Call moveNext function in a loop until it returns OPENDAQ_NO_MORE_ITEMS. Iteration
/// cannot be restarted. In this case a new iterator must be created.
/// <para/>
/// Example:
/// <code>
/// IIterator* it = ...;
///
/// while (it-&gt;moveNext() != OPENDAQ_NO_MORE_ITEMS)
/// {
///      IBaseObject* obj;
///      it-&gt;getCurrent(&amp;obj);
///      // do something with obj
/// }
  /// </code>
/// </remarks>
[Guid("f3b87158-f4cd-5890-9476-3c0e315c56d9")]
public class Iterator<TValue> : BaseObject, IEnumerator<TValue>
    where TValue : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawIterator _rawIterator;

    internal Iterator(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawIterator = Marshal.PtrToStructure<RawIterator>(objVirtualTable);
    }

    #region properties

    /// <summary>Gets the object at current iterator position.</summary>
    public TValue Current
    {
        get
        {
            //native output argument
            IntPtr objPtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawIterator.GetCurrent(base.NativePointer, out objPtr);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (objPtr == IntPtr.Zero)
            {
                return default;
            }

            return BaseObject.CreateInstance<TValue>(objPtr, incrementReference: false);
        }
    }

    #endregion properties

    /// <summary>Moves iterator to next position.</summary>
    public bool MoveNext()
    {
        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawIterator.MoveNext(base.NativePointer);

            if (errorCode == ErrorCode.OPENDAQ_ERR_NOMOREITEMS)
            {
                return false;
            }
            else if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }

            return (errorCode != ErrorCode.OPENDAQ_NO_MORE_ITEMS);
        }
    }

    #region IEnumerator<TValue> implementation

    /// <inheritdoc/>
    TValue IEnumerator<TValue>.Current => this.Current;

    #endregion IEnumerator<TValue> implementation

    #region IEnumerator implementation

    /// <inheritdoc/>
    object IEnumerator.Current => this.Current;

    /// <inheritdoc/>
    bool IEnumerator.MoveNext() => this.MoveNext();

    /// <inheritdoc/>
    void IEnumerator.Reset()
    {
        //does nothing for now, since we cannot reset IIterator as if newly created
        //throw new NotImplementedException();
    }

    #endregion IEnumerator implementation
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
}

#endregion Class Factory
