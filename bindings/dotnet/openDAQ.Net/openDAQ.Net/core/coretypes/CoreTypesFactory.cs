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


namespace Daq.Core.Types;


/// <summary>Factory functions of the &apos;CoreTypes&apos; library.</summary>
public static partial class CoreTypesFactory
{
    //void daqCoreTypesGetVersion(unsigned int* major, unsigned int* minor, unsigned int* revision); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqCoreTypesGetVersion(out int major, out int minor, out int revision);

    /// <summary>
    /// Gets the SDK version of the &apos;CoreTypes&apos; library.
    /// </summary>
    public static Version SdkVersion
    {
        get
        {
            //get the SDK version
            daqCoreTypesGetVersion(out int major, out int minor, out int revision);

            //create and return object
            return new Version(major, minor, revision);
        }
    }


    #region ErrorInfo

    // //void daqSetErrorInfo(daq.IErrorInfo* errorInfo); cdecl;
    // [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    // private static extern void daqSetErrorInfo(IntPtr errorInfo);

    // public static void DaqSetErrorInfo(ErrorInfo errorInfo)
    // {
    //     //call native function
    //     daqSetErrorInfo(errorInfo.NativePointer);
    // }

    //void daqGetErrorInfo(daq.IErrorInfo** errorInfo); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqGetErrorInfo(out IntPtr errorInfo);

    public static ErrorInfo DaqGetErrorInfo()
    {
        //native output argument
        IntPtr nativePtr;

        //call native function
        daqGetErrorInfo(out nativePtr);

        if (nativePtr == IntPtr.Zero)
        {
            return null;
        }

        //create and return object
        return new ErrorInfo(nativePtr, incrementReference: false);
    }

    //void daqClearErrorInfo(); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern void daqClearErrorInfo();

    public static void DaqClearErrorInfo()
    {
        //call native function
        daqClearErrorInfo();
    }

    #endregion ErrorInfo


    /// <summary>
    /// Gets the cast property value object.
    /// </summary>
    /// <param name="propertyValue">The property value.</param>
    /// <param name="propertyType">Type of the property.</param>
    /// <returns></returns>
    public static BaseObject GetPropertyValueObject(BaseObject propertyValue, CoreType propertyType)
    {
        switch (propertyType)
        {
            case CoreType.ctBool:            return propertyValue.Cast<BoolObject>();
            case CoreType.ctInt:             return propertyValue.Cast<IntegerObject>();
            case CoreType.ctFloat:           return propertyValue.Cast<FloatObject>();
            case CoreType.ctString:          return propertyValue.Cast<StringObject>();
            //case CoreType.ctList:          return null;
            //case CoreType.ctDict:          return null;
            //case CoreType.ctRatio:         return null;
            //case CoreType.ctProc:          return null;
            //case CoreType.ctObject:        return null;
            //case CoreType.ctBinaryData:    return null;
            //case CoreType.ctFunc:          return null;
            //case CoreType.ctComplexNumber: return null;
            //case CoreType.ctStruct:        return null;
            //case CoreType.ctEnumeration:   return null;
            //case CoreType.ctUndefined:     return null;
            default:                         return $"<{propertyType}>"; //StringObject ;)
        }
    }
}
