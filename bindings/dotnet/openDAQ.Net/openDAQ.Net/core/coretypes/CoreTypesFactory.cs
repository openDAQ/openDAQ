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
            case CoreType.ctList:            return propertyValue.Cast<ListObject<BaseObject>>();
            case CoreType.ctDict:            return propertyValue.Cast<DictObject<BaseObject, BaseObject>>();
            case CoreType.ctRatio:           return propertyValue.Cast<Ratio>();
            case CoreType.ctProc:            return propertyValue.Cast<Procedure>();
            case CoreType.ctObject:          return propertyValue.Cast<BaseObject>();
            //case CoreType.ctBinaryData:      return propertyValue.Cast<>();
            case CoreType.ctFunc:            return propertyValue.Cast<Function>();
            //case CoreType.ctComplexNumber:   return propertyValue.Cast<>();
            case CoreType.ctStruct:          return propertyValue.Cast<Struct>();
            case CoreType.ctEnumeration:     return propertyValue.Cast<Enumeration>();
            //case CoreType.ctUndefined:       return propertyValue.Cast<>();
            default:                         return $"<{propertyType}>"; //StringObject ;)
        }
    }

    #region NumberObject

    /// <summary>
    /// Creates the <see cref="NumberObject"/> from the given <c>double</c> value.
    /// </summary>
    /// <param name="obj">The <see cref="NumberObject"/> (<c>null</c> on error).</param>
    /// <param name="value">The value.</param>
    /// <returns><c>ErrorCode.OPENDAQ_SUCCESS</c> if the creation succeeded; otherwise the <see cref="ErrorCode"/>.</returns>
    public static ErrorCode CreateNumberObject(out NumberObject obj, double value)
    {
        try
        {
            //cast the value
            obj = ((FloatObject)value).Cast<NumberObject>();
        }
        catch (OpenDaqException ex)
        {
            //initialize output argument
            obj = default;
            return ex.ErrorCode;
        }

        return ErrorCode.OPENDAQ_SUCCESS;
    }

    /// <summary>
    /// Creates the <see cref="NumberObject"/> from the given <c>double</c> value.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <returns>The <see cref="NumberObject"/>.</returns>
    public static NumberObject CreateNumberObject(double value)
    {
        //cast and return object
        return ((FloatObject)value).Cast<NumberObject>();
    }

    /// <summary>
    /// Creates the <see cref="NumberObject"/> from the given <c>long</c> value.
    /// </summary>
    /// <param name="obj">The <see cref="NumberObject"/> (<c>null</c> on error).</param>
    /// <param name="value">The value.</param>
    /// <returns><c>ErrorCode.OPENDAQ_SUCCESS</c> if the creation succeeded; otherwise the <see cref="ErrorCode"/>.</returns>
    public static ErrorCode CreateNumberObject(out NumberObject obj, long value)
    {
        try
        {
            //cast the value
            obj = ((IntegerObject)value).Cast<NumberObject>();
        }
        catch (OpenDaqException ex)
        {
            //initialize output argument
            obj = default;
            return ex.ErrorCode;
        }

        return ErrorCode.OPENDAQ_SUCCESS;
    }

    /// <summary>
    /// Creates the <see cref="NumberObject"/> from the given <c>long</c> value.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <returns>The <see cref="NumberObject"/>.</returns>
    public static NumberObject CreateNumberObject(long value)
    {
        //cast and return object
        return ((IntegerObject)value).Cast<NumberObject>();
    }

    #endregion NumberObject
}
