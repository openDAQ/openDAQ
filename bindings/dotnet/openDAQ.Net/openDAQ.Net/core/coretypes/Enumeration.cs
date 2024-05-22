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
//     RTGen (CSharpGenerator v1.0.0) on 14.05.2024 09:39:36.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawEnumeration : RawBaseObject
{
    //ErrorCode getEnumerationType(daq.IEnumerationType** type); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetEnumerationType;
    //ErrorCode getValue(daq.IString** value); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetValue;
    //ErrorCode getIntValue(daq.Int* value); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out long, ErrorCode> GetIntValue;
}

/// <summary>
/// Enumerations are immutable objects that encapsulate a value within a predefined set of named integral constants.
/// These constants are predefined in an Enumeration type with the same name as the Enumeration.
/// </summary>
/// <remarks>
/// The Enumeration types are stored within a Type manager. In any given instance of openDAQ, a single Type manager should
/// exist that is part of its Context.
/// <para/>
/// When creating an Enumeration object, the Type manager is used to validate the given enumerator value name against the
/// Enumeration type stored within the manager. If no type with the given Enumeration name is currently stored,
/// construction of the Enumeration object will fail. Similarly, if the provided enumerator value name is not part of
/// the Enumeration type, the construction of the Enumeration object will also fail.
/// <para/>
/// Since the Enumerations objects are immutable the value of an existing Enumeration object cannot be modified.
/// However, the Enumeration object encapsulated by a smart pointer of the corresponding type can be replaced
/// with a newly created one. This replacement is accomplished using the assignment operator with the right
/// operand being a constant string literal containing the enumerator value name valid for the Enumeration type
/// of the original Enumeration object.
/// </remarks>
[Guid("5e7d128c-87ed-5fe3-9480-ccb7e7cf8f49")]
public class Enumeration : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawEnumeration _rawEnumeration;

    internal Enumeration(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawEnumeration = Marshal.PtrToStructure<RawEnumeration>(objVirtualTable);
    }

    /// <summary>Gets the Enumeration&apos;s type.</summary>
    public EnumerationType EnumerationType
    {
        get
        {
            //native output argument
            IntPtr typePtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawEnumeration.GetEnumerationType(base.NativePointer, out typePtr);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return new EnumerationType(typePtr, incrementReference: false);
        }
    }

    /// <summary>Gets the Enumeration value as String containing the name of the enumerator constant.</summary>
    public string Value
    {
        get
        {
            //native output argument
            IntPtr valuePtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawEnumeration.GetValue(base.NativePointer, out valuePtr);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (valuePtr == IntPtr.Zero)
            {
                return null;
            }

            using var value = new StringObject(valuePtr, incrementReference: false);
            return value;
        }
    }

    /// <summary>Gets the Enumeration value as Integer enumerator constant.</summary>
    public long IntValue
    {
        get
        {
            //native output argument
            long value;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawEnumeration.GetIntValue(base.NativePointer, out value);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return value;
        }
    }

    #region operators

    //implicit cast operators 'Daq.Core.Types.Enumeration' to/from 'string'

    /// <summary>Performs an implicit conversion from <see cref="Daq.Core.Types.Enumeration"/> to <see cref="string"/>.</summary>
    /// <param name="value">The SDK <c>Enumeration</c>.</param>
    /// <returns>The managed <c>string</c> value.</returns>
    public static implicit operator string(Enumeration value) => value.Value;

    /// <summary>Determines whether this instance and a specified <c>string</c>, have the same value.</summary>
    /// <param name="other">The other <c>string</c> to compare to this instance.</param>
    /// <returns><c>true</c> if the other <c>string</c> value is the same as this instance; otherwise, <c>false</c>.</returns>
    public bool Equals(string other) => ((string)this).Equals(other, StringComparison.Ordinal);

    //implicit cast operators 'Daq.Core.Types.Enumeration' to/from 'long'

    /// <summary>Performs an implicit conversion from <see cref="Daq.Core.Types.Enumeration"/> to <see cref="long"/>.</summary>
    /// <param name="value">The SDK <c>Enumeration</c>.</param>
    /// <returns>The managed <c>long</c> value.</returns>
    public static implicit operator long(Enumeration value) => value.IntValue;

    /// <summary>Determines whether this instance and a specified <c>long</c>, have the same value.</summary>
    /// <param name="other">The other <c>long</c> to compare to this instance.</param>
    /// <returns><c>true</c> if the other <c>long</c> value is the same as this instance; otherwise, <c>false</c>.</returns>
    public bool Equals(long other) => ((long)this).Equals(other);

    #endregion operators
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
    //ErrorCode createEnumeration(daq.IEnumeration** obj, daq.IString* name, daq.IString* value, daq.ITypeManager* typeManager); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createEnumeration(out IntPtr obj, IntPtr name, IntPtr value, IntPtr typeManager);

    public static ErrorCode CreateEnumeration(out Enumeration obj, string name, string value, TypeManager typeManager)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;
        using var valuePtr = (StringObject)value;

        //call native function
        ErrorCode errorCode = createEnumeration(out objPtr, namePtr.NativePointer, valuePtr.NativePointer, typeManager.NativePointer);

        if (Daq.Core.Types.Result.Succeeded(errorCode))
        {
            //create object
            obj = new Enumeration(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static Enumeration CreateEnumeration(string name, string value, TypeManager typeManager)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;
        using var valuePtr = (StringObject)value;

        //call native function
        ErrorCode errorCode = createEnumeration(out objPtr, namePtr.NativePointer, valuePtr.NativePointer, typeManager.NativePointer);

        if (Daq.Core.Types.Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new Enumeration(objPtr, incrementReference: false);
    }


    //ErrorCode createEnumerationWithIntValue(daq.IEnumeration** obj, daq.IString* name, daq.IInteger* value, daq.ITypeManager* typeManager); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createEnumerationWithIntValue(out IntPtr obj, IntPtr name, IntPtr value, IntPtr typeManager);

    public static ErrorCode CreateEnumerationWithIntValue(out Enumeration obj, string name, long value, TypeManager typeManager)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;
        using var valuePtr = (IntegerObject)value;

        //call native function
        ErrorCode errorCode = createEnumerationWithIntValue(out objPtr, namePtr.NativePointer, valuePtr.NativePointer, typeManager.NativePointer);

        if (Daq.Core.Types.Result.Succeeded(errorCode))
        {
            //create object
            obj = new Enumeration(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static Enumeration CreateEnumerationWithIntValue(string name, long value, TypeManager typeManager)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;
        using var valuePtr = (IntegerObject)value;

        //call native function
        ErrorCode errorCode = createEnumerationWithIntValue(out objPtr, namePtr.NativePointer, valuePtr.NativePointer, typeManager.NativePointer);

        if (Daq.Core.Types.Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new Enumeration(objPtr, incrementReference: false);
    }


    //ErrorCode createEnumerationWithType(daq.IEnumeration** obj, daq.IEnumerationType* type, daq.IString* value); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createEnumerationWithType(out IntPtr obj, IntPtr type, IntPtr value);

    public static ErrorCode CreateEnumerationWithType(out Enumeration obj, EnumerationType type, string value)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var valuePtr = (StringObject)value;

        //call native function
        ErrorCode errorCode = createEnumerationWithType(out objPtr, type.NativePointer, valuePtr.NativePointer);

        if (Daq.Core.Types.Result.Succeeded(errorCode))
        {
            //create object
            obj = new Enumeration(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static Enumeration CreateEnumerationWithType(EnumerationType type, string value)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var valuePtr = (StringObject)value;

        //call native function
        ErrorCode errorCode = createEnumerationWithType(out objPtr, type.NativePointer, valuePtr.NativePointer);

        if (Daq.Core.Types.Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new Enumeration(objPtr, incrementReference: false);
    }


    //ErrorCode createEnumerationWithIntValueAndType(daq.IEnumeration** obj, daq.IEnumerationType* type, daq.IInteger* value); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createEnumerationWithIntValueAndType(out IntPtr obj, IntPtr type, IntPtr value);

    public static ErrorCode CreateEnumerationWithIntValueAndType(out Enumeration obj, EnumerationType type, long value)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var valuePtr = (IntegerObject)value;

        //call native function
        ErrorCode errorCode = createEnumerationWithIntValueAndType(out objPtr, type.NativePointer, valuePtr.NativePointer);

        if (Daq.Core.Types.Result.Succeeded(errorCode))
        {
            //create object
            obj = new Enumeration(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static Enumeration CreateEnumerationWithIntValueAndType(EnumerationType type, long value)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var valuePtr = (IntegerObject)value;

        //call native function
        ErrorCode errorCode = createEnumerationWithIntValueAndType(out objPtr, type.NativePointer, valuePtr.NativePointer);

        if (Daq.Core.Types.Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new Enumeration(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
