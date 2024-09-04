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
//     RTGen (CSharpGenerator v1.0.0) on 04.09.2024 17:45:24.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawStringObject : RawBaseObject
{
    //ErrorCode getCharPtr(daq.ConstCharPtr* value); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetCharPtr;
    //ErrorCode getLength(daq.SizeT* size); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out nuint, ErrorCode> GetLength;
}

[Guid("d2ed1120-f7ff-556f-a98d-3f3edf1a3874")]
public class StringObject : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawStringObject _rawStringObject;

    internal StringObject(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawStringObject = Marshal.PtrToStructure<RawStringObject>(objVirtualTable);
    }

    #region properties

    /// <summary>Gets a string value stored in the object.</summary>
    /// <remarks>
    /// Call this method to extract the string value that is stored in the object. Method extracts the
    /// value as a pointer to 8-bit char type.
    /// </remarks>
    public string CharPtr
    {
        get
        {
            //native output argument
            IntPtr value;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawStringObject.GetCharPtr(base.NativePointer, out value);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return Marshal.PtrToStringAnsi(value);
        }
    }

    /// <summary>Gets length of string.</summary>
    /// <remarks>
    /// Call this method to get the length of the string. Null char terminator is not included in
    /// the size of the string.
    /// </remarks>
    public nuint Length
    {
        get
        {
            //native output argument
            nuint size;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawStringObject.GetLength(base.NativePointer, out size);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return size;
        }
    }

    #endregion properties

    #region operators

    //implicit cast operators 'Daq.Core.Types.StringObject' to/from 'string'

    /// <summary>Performs an implicit conversion from <see cref="string"/> to <see cref="Daq.Core.Types.StringObject"/>.</summary>
    /// <param name="value">The managed <c>string</c> value.</param>
    /// <returns>The SDK <c>StringObject</c>.</returns>
    public static implicit operator StringObject(string value) => CoreTypesFactory.CreateString(value);

    /// <summary>Performs an implicit conversion from <see cref="Daq.Core.Types.StringObject"/> to <see cref="string"/>.</summary>
    /// <param name="value">The SDK <c>StringObject</c>.</param>
    /// <returns>The managed <c>string</c> value.</returns>
    public static implicit operator string(StringObject value) => value?.CharPtr ?? default(string);

    /// <summary>Determines whether this instance and a specified <c>string</c>, have the same value.</summary>
    /// <param name="other">The other <c>string</c> to compare to this instance.</param>
    /// <returns><c>true</c> if the other <c>string</c> value is the same as this instance; otherwise, <c>false</c>.</returns>
    public bool Equals(string other) => ((string)this).Equals(other, StringComparison.Ordinal);

    #endregion operators
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
    //ErrorCode createString(daq.IString** obj, daq.ConstCharPtr str); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private static extern ErrorCode createString(out IntPtr obj, string str);

    public static ErrorCode CreateString(out StringObject obj, string str)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createString(out objPtr, str);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new StringObject(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static StringObject CreateString(string str)
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createString(out objPtr, str);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new StringObject(objPtr, incrementReference: false);
    }


    //ErrorCode createStringN(daq.IString** obj, daq.ConstCharPtr str, daq.SizeT length); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private static extern ErrorCode createStringN(out IntPtr obj, string str, nuint length);

    public static ErrorCode CreateStringN(out StringObject obj, string str, nuint length)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createStringN(out objPtr, str, length);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new StringObject(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static StringObject CreateStringN(string str, nuint length)
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createStringN(out objPtr, str, length);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new StringObject(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
