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
//     RTGen (CSharpGenerator v1.0.0) on 04.09.2024 17:45:18.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawDaqEvent : RawBaseObject
{
    //ErrorCode addHandler(daq.IEventHandler* eventHandler); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> AddHandler;
    //ErrorCode removeHandler(daq.IEventHandler* eventHandler); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> RemoveHandler;
    //ErrorCode trigger(daq.IBaseObject* sender, daq.IEventArgs* args); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, IntPtr, ErrorCode> Trigger;
    //ErrorCode clear(); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, ErrorCode> Clear;
    //ErrorCode getSubscriberCount(daq.SizeT* count); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out nuint, ErrorCode> GetSubscriberCount;
    //ErrorCode getSubscribers(daq.IList** subscribers); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> GetSubscribers;
    //ErrorCode mute(); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, ErrorCode> Mute;
    //ErrorCode unmute(); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, ErrorCode> Unmute;
    //ErrorCode muteListener(daq.IEventHandler* eventHandler); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> MuteListener;
    //ErrorCode unmuteListener(daq.IEventHandler* eventHandler); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, IntPtr, ErrorCode> UnmuteListener;
}

[Guid("82774c35-1638-5228-9a72-8dddfdf10c10")]
public class DaqEvent/*<TSender, TEventArgs>*/ : BaseObject
    //where TSender : BaseObject
    //where TEventArgs : EventArgs
{
    //type-casted base._virtualTable
    private readonly RawDaqEvent _rawDaqEvent;

    internal DaqEvent(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawDaqEvent = Marshal.PtrToStructure<RawDaqEvent>(objVirtualTable);
    }

    #region properties

    public nuint SubscriberCount
    {
        get
        {
            //native output argument
            nuint count;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawDaqEvent.GetSubscriberCount(base.NativePointer, out count);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return count;
        }
    }

    public IListObject<BaseObject> Subscribers
    {
        get
        {
            //native output argument
            IntPtr subscribersPtr;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawDaqEvent.GetSubscribers(base.NativePointer, out subscribersPtr);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            // validate pointer
            if (subscribersPtr == IntPtr.Zero)
            {
                return default;
            }

            return new ListObject<BaseObject>(subscribersPtr, incrementReference: false);
        }
    }

    #endregion properties

    public void AddHandler(DaqEventHandler eventHandler)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDaqEvent.AddHandler(base.NativePointer, eventHandler);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    public void RemoveHandler(DaqEventHandler eventHandler)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDaqEvent.RemoveHandler(base.NativePointer, eventHandler);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    public void Trigger(BaseObject sender, DaqEventArgs args)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDaqEvent.Trigger(base.NativePointer, sender, args);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    public void Clear()
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDaqEvent.Clear(base.NativePointer);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    public void Mute()
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDaqEvent.Mute(base.NativePointer);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    public void Unmute()
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDaqEvent.Unmute(base.NativePointer);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    public void MuteListener(DaqEventHandler eventHandler)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDaqEvent.MuteListener(base.NativePointer, eventHandler);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }
    }

    public void UnmuteListener(DaqEventHandler eventHandler)
    {
        unsafe //use native method pointer
        {
            //call native method
            ErrorCode errorCode = (ErrorCode)_rawDaqEvent.UnmuteListener(base.NativePointer, eventHandler);

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
    //ErrorCode createEvent(daq.IEvent** obj); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createEvent(out IntPtr obj);

    public static ErrorCode CreateEvent(out DaqEvent obj)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createEvent(out objPtr);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new DaqEvent(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static DaqEvent CreateEvent()
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createEvent(out objPtr);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new DaqEvent(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
