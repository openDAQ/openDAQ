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
//     RTGen (CSharpGenerator v1.0.0) on 25.06.2024 08:46:41.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawDaqEventArgs : RawBaseObject
{
    //ErrorCode getEventId(daq.Int* id); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out long, ErrorCode> GetEventId;
    //ErrorCode getEventName(daq.IString** name); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetEventName;
}

[Guid("81d0979c-1fa7-51f8-80fb-44216a6f8d33")]
public class DaqEventArgs : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawDaqEventArgs _rawDaqEventArgs;

    internal DaqEventArgs(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawDaqEventArgs = Marshal.PtrToStructure<RawDaqEventArgs>(objVirtualTable);
    }

    #region properties

    public long EventId
    {
        get
        {
            //native output argument
            long id;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawDaqEventArgs.GetEventId(base.NativePointer, out id);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return id;
        }
    }

    public string EventName
    {
        get
        {
            //native output argument
            IntPtr namePtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawDaqEventArgs.GetEventName(base.NativePointer, out namePtr);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (namePtr == IntPtr.Zero)
            {
                return default;
            }

            using var name = new StringObject(namePtr, incrementReference: false);
            return name;
        }
    }

    #endregion properties
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
    //ErrorCode createEventArgs(daq.IEventArgs** obj, daq.Int eventId, daq.IString* eventName); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createEventArgs(out IntPtr obj, long eventId, IntPtr eventName);

    public static ErrorCode CreateEventArgs(out DaqEventArgs obj, long eventId, string eventName)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var eventNamePtr = (StringObject)eventName;

        //call native function
        ErrorCode errorCode = createEventArgs(out objPtr, eventId, eventNamePtr.NativePointer);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new DaqEventArgs(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static DaqEventArgs CreateEventArgs(long eventId, string eventName)
    {
        //native output argument
        IntPtr objPtr;

        //cast .NET argument to SDK object
        using var eventNamePtr = (StringObject)eventName;

        //call native function
        ErrorCode errorCode = createEventArgs(out objPtr, eventId, eventNamePtr.NativePointer);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new DaqEventArgs(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
