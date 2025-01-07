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
internal unsafe class RawIterable : RawBaseObject
{
    //ErrorCode createStartIterator(daq.IIterator** iterator); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> CreateStartIterator;
    //ErrorCode createEndIterator(daq.IIterator** iterator); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> CreateEndIterator;
}

/// <summary>An iterable object can construct iterators and use them to iterate through items.</summary>
/// <remarks>
/// Use this interface to get the start and end iterators. Use iterators to iterate through
/// available items. Containers such as lists and dictionaries usually implement this interface.
/// </remarks>
[Guid("ec09f2e5-614d-5780-81cb-ece8ecb2655b")]
public class Iterable<TValue> : BaseObject
    where TValue : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawIterable _rawIterable;

    internal Iterable(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawIterable = Marshal.PtrToStructure<RawIterable>(objVirtualTable);
    }

    /// <summary>Creates and returns the object&apos;s start iterator.</summary>
    /// <returns>The object&apos;s start iterator.</returns>
    public IEnumerator<TValue> CreateStartIterator()
    {
        //native output argument
        IntPtr iteratorPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawIterable.CreateStartIterator(base.NativePointer, out iteratorPtr);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        // validate pointer
        if (iteratorPtr == IntPtr.Zero)
        {
            return default;
        }

        return new Iterator<TValue>(iteratorPtr, incrementReference: false);
    }

    /// <summary>Creates and returns the object&apos;s end iterator.</summary>
    /// <returns>The object&apos;s end iterator.</returns>
    public IEnumerator<TValue> CreateEndIterator()
    {
        //native output argument
        IntPtr iteratorPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawIterable.CreateEndIterator(base.NativePointer, out iteratorPtr);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        // validate pointer
        if (iteratorPtr == IntPtr.Zero)
        {
            return default;
        }

        return new Iterator<TValue>(iteratorPtr, incrementReference: false);
    }
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
}

#endregion Class Factory
