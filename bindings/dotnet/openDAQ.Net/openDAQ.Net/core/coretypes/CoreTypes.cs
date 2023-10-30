/*
 * Copyright 2022-2023 Blueberry d.o.o.
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


namespace Daq.Core.Types;


public static class CoreTypes
{
    #region DLL Import

    [DllImport(CoreTypesDllInfo.FileName, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode daqDuplicateCharPtrN(string source, nuint size, out IntPtr dest);

    [DllImport(CoreTypesDllInfo.FileName, CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode daqDuplicateCharPtr(string source, out IntPtr dest);

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]//unfortunately (out BaseObject baseObject); not working
    private static extern int daqCycleDetectEnter(IntPtr baseObjectPtr);

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqCycleDetectLeave(IntPtr baseObjectPtr);

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode daqUnregisterSerializerFactory(string id);

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqTrackObject(IntPtr baseObjectPtr);

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqUntrackObject(IntPtr baseObjectPtr);

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ulong daqGetTrackedObjectCount();

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqPrintTrackedObjects();

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqClearTrackedObjects();

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern bool daqIsTrackingObjects();

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqPrepareHeapAlloc();

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqFreeMemory(IntPtr ptr);

    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr daqAllocateMemory(ulong len);

    #endregion DLL Import

    public static void FreeMemory(IntPtr ptr)
    {
        daqFreeMemory(ptr);
    }

    public static IntPtr AllocateMemory(ulong len)
    {
        IntPtr ptr = daqAllocateMemory(len);
        return ptr;
    }

    public static ErrorCode DuplicateCharPtrN(string source, nuint size, out string dest)
    {
        IntPtr ptr;
        ErrorCode errorCode = daqDuplicateCharPtrN(source, size, out ptr);
        dest = Marshal.PtrToStringAnsi(ptr);
        return errorCode;
    }

    public static ErrorCode DuplicateCharPtr(string source, out string dest)
    {
        IntPtr ptr;
        ErrorCode errorCode = daqDuplicateCharPtr(source, out ptr);
        dest = Marshal.PtrToStringAnsi(ptr);
        return errorCode;
    }

    public static int CycleDetectEnter(BaseObject baseObject)
    {

        unsafe
        {
            int errorCode = daqCycleDetectEnter(baseObject.NativePointer);
            return errorCode;
        }
    }

    public static void CycleDetectLeave(BaseObject baseObject)
    {
        unsafe
        {
            daqCycleDetectLeave(baseObject.NativePointer);
        }
    }

    public static ErrorCode UnregisterSerializerFactory(string id)
    {
        ErrorCode errorCode = daqUnregisterSerializerFactory(id);
        return errorCode;
    }

    public static void TrackObject(BaseObject baseObject)
    {
        unsafe
        {
            daqTrackObject(baseObject.NativePointer);
        }
    }

    public static void UntrackObject(BaseObject baseObject)
    {
        if (baseObject is null)
            return;

        unsafe
        {
            daqUntrackObject(baseObject.NativePointer);
        }
    }

    public static ulong GetTrackedObjectCount()
    {
        ulong trackedObjectCount = daqGetTrackedObjectCount();
        if ((trackedObjectCount & 0x80000000UL) != 0UL)
            return 0UL;
        return trackedObjectCount;
    }

    public static void PrintTrackedObjects()
    {
        daqPrintTrackedObjects();
    }

    public static void ClearTrackedObjects()
    {
        daqClearTrackedObjects();
    }

    public static bool IsTrackingObjects()
    {
        return daqIsTrackingObjects();
    }

    public static void PrepareHeapAlloc()
    {
        daqPrepareHeapAlloc();
    }
}
