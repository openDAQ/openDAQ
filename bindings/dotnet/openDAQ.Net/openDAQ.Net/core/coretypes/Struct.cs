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
//     RTGen (CSharpGenerator v1.0.0) on 29.04.2024 15:46:02.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawStruct : RawBaseObject
{
    //ErrorCode getStructType(daq.IStructType** type); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetStructType;
    //ErrorCode getFieldNames(daq.IList** names); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetFieldNames;
    //ErrorCode getFieldValues(daq.IList** values); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetFieldValues;
    //ErrorCode get(daq.IString* name, daq.IBaseObject** field); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, out IntPtr, ErrorCode> Get;
    //ErrorCode getAsDictionary(daq.IDict** dictionary); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetAsDictionary;
    //ErrorCode hasField(daq.IString* name, daq.Bool* contains); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, out bool, ErrorCode> HasField;
}

/// <summary>
/// Structs are immutable objects that contain a set of key-value pairs. The key, as well as the types of each
/// associated value for each struct are defined in advance within a Struct type that has the same name as the Struct.
/// </summary>
/// <remarks>
/// The Struct types are stored within a Type manager. In any given instance of openDAQ, a single Type manager should
/// exist that is part of its Context.
/// <para/>
/// When creating a Struct, the Type manager is used to validate the given dictionary of keys and values against the
/// Struct type stored within the manager. If no type with the given Struct name is currently stored, a default type
/// is created using the Struct field names and values as its parameters. When creating a Struct, fields that are part
/// of the Struct type can be omitted. If so, they will be replaced by either <c>null</c> or, if provided by the Struct type,
/// the default value of the field.
/// <para/>
/// In the case that a field name is present that is not part of the struct type, or if the value type of the field does
/// not match, construction of the Struct will fail.
/// <para/>
/// NOTE: Field values of fields with the Core type <c>ctUndefined</c> can hold any value, regardless of its type.
/// <para/>
/// Structs are an openDAQ core type (ctStruct). Several objects in openDAQ such as an Unit, or DataDescriptor are Structs,
/// allowing for access to their fields through Struct methods. Such objects are, by definiton, immutable - their fields
/// cannot be modified. In order to change the value of a Struct-type object, a new Struct must be created.
/// <para/>
/// A Struct can only have fields of Core type: <c>ctBool</c>, <c>ctInt</c>, <c>ctFloat</c>, <c>ctString</c>, <c>ctList</c>, <c>ctDict</c>, <c>ctRatio</c>, <c>ctComplexNumber</c>,
/// <c>ctStruct</c>, or <c>ctUndefined</c>. Additionally, all Container types (<c>ctList</c>, <c>ctDict</c>) should only have values of the aforementioned
/// types.
/// </remarks>
[Guid("2b9f7790-512a-591e-86ac-886e9de68a52")]
public class Struct : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawStruct _rawStruct;

    internal Struct(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawStruct = Marshal.PtrToStructure<RawStruct>(objVirtualTable);
    }

    /// <summary>Gets the Struct&apos;s type.</summary>
    public StructType StructType
    {
        get
        {
            //native output argument
            IntPtr typePtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawStruct.GetStructType(base.NativePointer, out typePtr);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return new StructType(typePtr, incrementReference: false);
        }
    }

    /// <summary>Gets a list of all Struct field names.</summary>
    /// <remarks>
    /// The list of names will be of equal length to the list of values. Additionally, the name of a field at any given
    /// index corresponds to the value stored in the list of values.
    /// </remarks>
    public IListObject<StringObject> FieldNames
    {
        get
        {
            //native output argument
            IntPtr namesPtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawStruct.GetFieldNames(base.NativePointer, out namesPtr);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return new ListObject<StringObject>(namesPtr, incrementReference: false);
        }
    }

    /// <summary>Gets a list of all Struct field values.</summary>
    /// <remarks>
    /// The list of names will be of equal length to the list of values. Additionally, the name of a field at any given
    /// index corresponds to the value stored in the list of values.
    /// </remarks>
    public IListObject<BaseObject> FieldValues
    {
        get
        {
            //native output argument
            IntPtr valuesPtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawStruct.GetFieldValues(base.NativePointer, out valuesPtr);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return new ListObject<BaseObject>(valuesPtr, incrementReference: false);
        }
    }

    /// <summary>Gets the field names and values of the Struct as a Dictionary.</summary>
    public IDictObject<StringObject, BaseObject> AsDictionary
    {
        get
        {
            //native output argument
            IntPtr dictionaryPtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawStruct.GetAsDictionary(base.NativePointer, out dictionaryPtr);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return new DictObject<StringObject, BaseObject>(dictionaryPtr, incrementReference: false);
        }
    }
    /// <summary>Gets the value of a field with the given name.</summary>
    /// <param name="name">The name of the queried field.</param>
    /// <returns>The value of the field.</returns>
    public BaseObject Get(string name)
    {
        //native output argument
        IntPtr fieldPtr;

        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawStruct.Get(base.NativePointer, namePtr.NativePointer, out fieldPtr);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return new BaseObject(fieldPtr, incrementReference: false);
    }

    /// <summary>Checks whether a field with the given name exists in the Struct</summary>
    /// <param name="name">The name of the checked field.</param>
    /// <returns>True if the a field with <c>name</c> exists in the Struct; false otherwise.</returns>
    public bool HasField(string name)
    {
        //native output argument
        bool contains;

        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawStruct.HasField(base.NativePointer, namePtr.NativePointer, out contains);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return contains;
    }
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
    //ErrorCode createStruct(daq.IStruct** obj, daq.IString* name, daq.IDict* fields, daq.ITypeManager* typeManager); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createStruct(out IntPtr obj, IntPtr name, IntPtr fields, IntPtr typeManager);

    public static ErrorCode CreateStruct(out Struct obj, string name, IDictObject<BaseObject, BaseObject> fields, TypeManager typeManager)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;
        var fieldsPtr = (DictObject<BaseObject, BaseObject>)fields;

        //call native function
        ErrorCode errorCode = createStruct(out objPtr, namePtr.NativePointer, fieldsPtr.NativePointer, typeManager.NativePointer);

        if (Daq.Core.Types.Result.Succeeded(errorCode))
        {
            //create object
            obj = new Struct(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static Struct CreateStruct(string name, IDictObject<BaseObject, BaseObject> fields, TypeManager typeManager)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;
        var fieldsPtr = (DictObject<BaseObject, BaseObject>)fields;

        //call native function
        ErrorCode errorCode = createStruct(out objPtr, namePtr.NativePointer, fieldsPtr.NativePointer, typeManager.NativePointer);

        if (Daq.Core.Types.Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new Struct(objPtr, incrementReference: false);
    }


    //ErrorCode createStructFromBuilder(daq.IStruct** obj, daq.IStructBuilder* builder); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createStructFromBuilder(out IntPtr obj, IntPtr builder);

    public static ErrorCode CreateStructFromBuilder(out Struct obj, StructBuilder builder)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createStructFromBuilder(out objPtr, builder.NativePointer);

        if (Daq.Core.Types.Result.Succeeded(errorCode))
        {
            //create object
            obj = new Struct(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static Struct CreateStructFromBuilder(StructBuilder builder)
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createStructFromBuilder(out objPtr, builder.NativePointer);

        if (Daq.Core.Types.Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new Struct(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
