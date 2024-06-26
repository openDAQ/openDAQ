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
//     RTGen (CSharpGenerator v1.0.0) on 27.05.2024 12:24:02.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawProcedure : RawBaseObject
{
    //ErrorCode dispatch(daq.IBaseObject* params); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> Dispatch;
}

/// <summary>Holds a callback function without return value.</summary>
/// <remarks>
/// Represents a callable object without return value. The openDAQ SDK uses this interface when
/// it needs to make a call back to the client.
/// <para/>
/// Although the implementation of this interface is provided by openDAQ, C++ and other
/// bindings provide their implementation which allows passing function as a lambda
/// functions and other constructs.
/// <para/>
/// Available factories:
/// <code>
/// // Creates a new Procedure object. Throws exception if not successful.
/// IFunction* Procedure_Create(ProcCall value)
///
/// // Creates a new Procedure object. Returns error code if not successful.
/// ErrCode createProcedure(IFuncObject** obj, ProcCall value)
/// </code>
/// </remarks>
[Guid("36247e6d-6bdd-5964-857d-0fd296eeb5c3")]
public class Procedure : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawProcedure _rawProcedure;

    internal Procedure(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawProcedure = Marshal.PtrToStructure<RawProcedure>(objVirtualTable);
    }

    /// <summary>Calls the stored callback.</summary>
    /// <remarks>
    /// If the callback expects no parameters, the <c>params</c> parameter has to be <c>nullptr</c>. If it
    /// expects a single parameter, pass any openDAQ object as the <c>params</c> parameter.
    /// If it expects multiple parameters, pass an IList&lt;IBaseObject&gt; as the <c>params</c> parameter.
    /// </remarks>
    /// <param name="params">Parameters passed to the callback.</param>
    public void Dispatch(BaseObject @params)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawProcedure.Dispatch(base.NativePointer, @params.NativePointer);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
    //ErrorCode createProcedure(daq.IProcedure** obj, daq.ProcCall value); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createProcedure(out IntPtr obj, ProcCall value);

    public static ErrorCode CreateProcedure(out Procedure obj, ProcCall value)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createProcedure(out objPtr, value);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new Procedure(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static Procedure CreateProcedure(ProcCall value)
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createProcedure(out objPtr, value);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new Procedure(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
