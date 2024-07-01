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
//     RTGen (CSharpGenerator v1.0.0) on 25.06.2024 08:46:49.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawTypeManager : RawBaseObject
{
    //ErrorCode addType(daq.IType* type); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> AddType;
    //ErrorCode removeType(daq.IString* typeName); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> RemoveType;
    //ErrorCode getType(daq.IString* typeName, daq.IType** type); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, out IntPtr, ErrorCode> GetDaqType;
    //ErrorCode getTypes(daq.IList** types); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetTypes;
    //ErrorCode hasType(daq.IString* typeName, daq.Bool* hasType); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, out bool, ErrorCode> HasType;
}

/// <summary>
/// Container for Type objects. The Type manager is used when creating certain types of objects
/// (eg. Structs and Property object classes). The Types stored within the manager contain pre-defined fields,
/// as well as constraints specifying how objects should look.
/// </summary>
/// <remarks>
/// The currently available types in openDAQ that should be added to the Type manager are the Struct type
/// and the Property object class. The former is used to validate Structs when they are created, while the latter
/// contains pre-defined properties that are added to Property objects on construction.
/// <para/>
/// When adding a Property object class to the manager, they can form inheritance chains with one-another.
/// Thus, a parent of a given class must be added to the manager before any of its children. Likewise, a class
/// cannot be removed before its children are removed.
/// </remarks>
[Guid("ebd840c6-7e32-51f4-b063-63d0b09f4240")]
public class TypeManager : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawTypeManager _rawTypeManager;

    internal TypeManager(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawTypeManager = Marshal.PtrToStructure<RawTypeManager>(objVirtualTable);
    }

    #region properties

    /// <summary>Gets a list of all added Types.</summary>
    public IListObject<StringObject> Types
    {
        get
        {
            //native output argument
            IntPtr typesPtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawTypeManager.GetTypes(base.NativePointer, out typesPtr);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (typesPtr == IntPtr.Zero)
            {
                return default;
            }

            return new ListObject<StringObject>(typesPtr, incrementReference: false);
        }
    }

    #endregion properties

    /// <summary>Adds a type to the manager.</summary>
    /// <remarks>
    /// The type name must be unique and. If a Property object class specifies a parent class,
    /// then the parent class must be added before it.
    /// </remarks>
    /// <param name="type">The Type to be added.</param>
    /// <exception cref="OpenDaqException">
    /// <c>OpenDaqException(ErrorCode.OPENDAQ_ERR_ALREADYEXISTS)</c>
    /// if a type with the same name is already added.
    /// </exception>
    /// <exception cref="OpenDaqException">
    /// <c>OpenDaqException(ErrorCode.OPENDAQ_ERR_INVALIDPARAMETER)</c>
    /// if either the type name is an empty string.
    /// </exception>
    public void AddType(DaqType type)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawTypeManager.AddType(base.NativePointer, type.NativePointer);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Removes the type from the manager.</summary>
    /// <remarks>
    /// The removed class must not be a parent of another added class. If it is, those classes must be removed
    /// before it.
    /// </remarks>
    /// <param name="typeName">The type&apos;s name.</param>
    /// <exception cref="OpenDaqException">
    /// <c>OpenDaqException(ErrorCode.OPENDAQ_ERR_NOTFOUND)</c>
    /// if the class is not registered.
    /// </exception>
    public void RemoveType(string typeName)
    {
        //cast .NET argument to SDK object
        using var typeNamePtr = (StringObject)typeName;

        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawTypeManager.RemoveType(base.NativePointer, typeNamePtr.NativePointer);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    /// <summary>Gets an added Type by name.</summary>
    /// <param name="typeName">The Type&apos;s name.</param>
    /// <returns>The Type with name equal to <c>name</c>.</returns>
    /// <exception cref="OpenDaqException">
    /// <c>OpenDaqException(ErrorCode.OPENDAQ_ERR_NOTFOUND)</c>
    /// if a Type with the specified name is not added.
    /// </exception>
    public DaqType GetDaqType(string typeName)
    {
        //native output argument
        IntPtr typePtr;

        //cast .NET argument to SDK object
        using var typeNamePtr = (StringObject)typeName;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawTypeManager.GetDaqType(base.NativePointer, typeNamePtr.NativePointer, out typePtr);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        // validate pointer
        if (typePtr == IntPtr.Zero)
        {
            return default;
        }

        return new DaqType(typePtr, incrementReference: false);
    }

    /// <summary>Checks if a type with the specified name is already added.</summary>
    /// <param name="typeName">The name of the checked type.</param>
    /// <returns>True if the type is aready added to the manager; False otherwise.</returns>
    public bool HasType(string typeName)
    {
        //native output argument
        bool hasType;

        //cast .NET argument to SDK object
        using var typeNamePtr = (StringObject)typeName;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawTypeManager.HasType(base.NativePointer, typeNamePtr.NativePointer, out hasType);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        return hasType;
    }
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
    //ErrorCode createTypeManager(daq.ITypeManager** obj); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createTypeManager(out IntPtr obj);

    public static ErrorCode CreateTypeManager(out TypeManager obj)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createTypeManager(out objPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new TypeManager(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static TypeManager CreateTypeManager()
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createTypeManager(out objPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new TypeManager(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
