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
//     RTGen (CSharpGenerator v1.0.0) on 25.06.2024 08:46:40.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawErrorInfo : RawBaseObject
{
    //ErrorCode setMessage(daq.IString* message); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> SetMessage;
    //ErrorCode getMessage(daq.IString** message); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetMessage;
    //ErrorCode setSource(daq.IString* source); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> SetSource;
    //ErrorCode getSource(daq.IString** source); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetSource;
}

/// <summary>Contains detailed information about error.</summary>
/// <remarks>
/// Most of openDAQ&apos;s methods in interfaces return <c>ErrCode</c>. This is just an integer number. With <c>IErrorInfo</c>
/// interface it is possible to attach an error message and a source to the last error. The Interface function that
/// returns an error can create an object that implements this interface and attaches it to thread-local storage
/// using <c>daqSetErrorInfo</c> function. The Client can check the return value of an arbitrary interface function and
/// in case of an error, it can check if <c>IErrorInfo</c> is stored in thread-local storage using <c>daqGetErrorInfo</c> for
/// additional error information.
/// <para/>
/// <c>makeErrorInfo</c> automatically creates IErrorInfo and calls <c>daqSetErrorInfo</c>. In case of an error, <c>checkErrorInfo</c>
/// calls <c>daqGetErrorInfo</c> to get extended error information and throws an exception.
/// <para/>
/// Example:
/// <code>
/// ErrCode ISomeInterface::checkValue(Int value)
/// {
///     if (value &lt; 0)
///         return makeErrorInfo(OPENDAQ_ERR_INVALIDPARAMETER, &quot;Parameter should be &gt;= 0&quot;, nullptr);
///     return OPENDAQ_SUCCESS;
/// };
///
/// auto errCode = someInterface-&gt;checkValue(-1);
/// checkErrorInfo(errCode); // this will throw InvalidParameterException with above error message
/// </code>
/// </remarks>
[Guid("483b3446-8f45-53ce-b4ee-ec2b03cf6a4c")]
public class ErrorInfo : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawErrorInfo _rawErrorInfo;

    internal ErrorInfo(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawErrorInfo = Marshal.PtrToStructure<RawErrorInfo>(objVirtualTable);
    }

    #region properties

    /// <summary>Sets the message of the error.</summary>
    public string Message
    {
        get
        {
            //native output argument
            IntPtr messagePtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawErrorInfo.GetMessage(base.NativePointer, out messagePtr);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (messagePtr == IntPtr.Zero)
            {
                return default;
            }

            using var message = new StringObject(messagePtr, incrementReference: false);
            return message;
        }
        set
        {
            //cast .NET argument to SDK object
            using var messagePtr = (StringObject)value;

            unsafe //use native method pointer
            {
                //call native method
                ErrorCode errorCode = (ErrorCode)_rawErrorInfo.SetMessage(base.NativePointer, messagePtr.NativePointer);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }
        }
    }

    /// <summary>Sets the source of the error.</summary>
    public string Source
    {
        get
        {
            //native output argument
            IntPtr sourcePtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawErrorInfo.GetSource(base.NativePointer, out sourcePtr);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (sourcePtr == IntPtr.Zero)
            {
                return default;
            }

            using var source = new StringObject(sourcePtr, incrementReference: false);
            return source;
        }
        set
        {
            //cast .NET argument to SDK object
            using var sourcePtr = (StringObject)value;

            unsafe //use native method pointer
            {
                //call native method
                ErrorCode errorCode = (ErrorCode)_rawErrorInfo.SetSource(base.NativePointer, sourcePtr.NativePointer);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }
        }
    }

    #endregion properties
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
    //ErrorCode createErrorInfo(daq.IErrorInfo** obj); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createErrorInfo(out IntPtr obj);

    public static ErrorCode CreateErrorInfo(out ErrorInfo obj)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createErrorInfo(out objPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new ErrorInfo(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static ErrorInfo CreateErrorInfo()
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createErrorInfo(out objPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new ErrorInfo(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
