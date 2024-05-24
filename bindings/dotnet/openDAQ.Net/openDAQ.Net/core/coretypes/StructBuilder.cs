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
//     RTGen (CSharpGenerator v1.0.0) on 22.05.2024 13:58:37.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawStructBuilder : RawBaseObject
{
    //ErrorCode build(daq.IStruct** struct_); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> Build;
    //ErrorCode getStructType(daq.IStructType** type); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetStructType;
    //ErrorCode getFieldNames(daq.IList** names); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetFieldNames;
    //ErrorCode setFieldValues(daq.IList* values); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> SetFieldValues;
    //ErrorCode getFieldValues(daq.IList** values); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetFieldValues;
    //ErrorCode set(daq.IString* name, daq.IBaseObject* field); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, IntPtr, ErrorCode> Set;
    //ErrorCode get(daq.IString* name, daq.IBaseObject** field); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, out IntPtr, ErrorCode> Get;
    //ErrorCode hasField(daq.IString* name, daq.Bool* contains); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, out bool, ErrorCode> HasField;
    //ErrorCode getAsDictionary(daq.IDict** dictionary); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetAsDictionary;
}

/// <summary>
/// Builder component of Struct objects. Contains setter methods to configure the Struct parameters, and a
/// <c>build</c> method that builds the Struct object.
/// </summary>
[Guid("a1d23dad-e61a-5ff5-b3b7-fe8932c70cd0")]
public class StructBuilder : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawStructBuilder _rawStructBuilder;

    internal StructBuilder(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawStructBuilder = Marshal.PtrToStructure<RawStructBuilder>(objVirtualTable);
    }

    #region properties

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
                ErrorCode errorCode = (ErrorCode)_rawStructBuilder.GetStructType(base.NativePointer, out typePtr);

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
                ErrorCode errorCode = (ErrorCode)_rawStructBuilder.GetFieldNames(base.NativePointer, out namesPtr);

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
                ErrorCode errorCode = (ErrorCode)_rawStructBuilder.GetFieldValues(base.NativePointer, out valuesPtr);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return new ListObject<BaseObject>(valuesPtr, incrementReference: false);
        }
        set
        {
            //cast .NET argument to SDK object
            var valuesPtr = (ListObject<BaseObject>)value;

            unsafe //use native method pointer
            {
                //call native method
                ErrorCode errorCode = (ErrorCode)_rawStructBuilder.SetFieldValues(base.NativePointer, valuesPtr.NativePointer);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }
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
                ErrorCode errorCode = (ErrorCode)_rawStructBuilder.GetAsDictionary(base.NativePointer, out dictionaryPtr);

                if (Daq.Core.Types.Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return new DictObject<StringObject, BaseObject>(dictionaryPtr, incrementReference: false);
        }
    }

    #endregion properties

    /// <summary>Builds and returns a Struct object using the currently set values of the Builder.</summary>
    /// <returns>The built Struct.</returns>
    public Struct Build()
    {
        //native output argument
        IntPtr struct_Ptr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawStructBuilder.Build(base.NativePointer, out struct_Ptr);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return new Struct(struct_Ptr, incrementReference: false);
    }

    /// <summary>Sets the value of a field with the given name.</summary>
    /// <param name="name">The name of the queried field.</param>
    /// <param name="field">The value of the field.</param>
    public void Set(string name, BaseObject field)
    {
        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;

        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawStructBuilder.Set(base.NativePointer, namePtr.NativePointer, field.NativePointer);

            if (Daq.Core.Types.Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
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
            ErrorCode errorCode = (ErrorCode)_rawStructBuilder.Get(base.NativePointer, namePtr.NativePointer, out fieldPtr);

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
            ErrorCode errorCode = (ErrorCode)_rawStructBuilder.HasField(base.NativePointer, namePtr.NativePointer, out contains);

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
    //ErrorCode createStructBuilder(daq.IStructBuilder** obj, daq.IString* name, daq.ITypeManager* typeManager); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createStructBuilder(out IntPtr obj, IntPtr name, IntPtr typeManager);

    public static ErrorCode CreateStructBuilder(out StructBuilder obj, string name, TypeManager typeManager)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;

        //call native function
        ErrorCode errorCode = createStructBuilder(out objPtr, namePtr.NativePointer, typeManager.NativePointer);

        if (Daq.Core.Types.Result.Succeeded(errorCode))
        {
            //create object
            obj = new StructBuilder(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static StructBuilder CreateStructBuilder(string name, TypeManager typeManager)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var namePtr = (StringObject)name;

        //call native function
        ErrorCode errorCode = createStructBuilder(out objPtr, namePtr.NativePointer, typeManager.NativePointer);

        if (Daq.Core.Types.Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new StructBuilder(objPtr, incrementReference: false);
    }


    //ErrorCode createStructBuilderFromStruct(daq.IStructBuilder** obj, daq.IStruct* struct_); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createStructBuilderFromStruct(out IntPtr obj, IntPtr struct_);

    public static ErrorCode CreateStructBuilderFromStruct(out StructBuilder obj, Struct struct_)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createStructBuilderFromStruct(out objPtr, struct_.NativePointer);

        if (Daq.Core.Types.Result.Succeeded(errorCode))
        {
            //create object
            obj = new StructBuilder(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static StructBuilder CreateStructBuilderFromStruct(Struct struct_)
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createStructBuilderFromStruct(out objPtr, struct_.NativePointer);

        if (Daq.Core.Types.Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new StructBuilder(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
