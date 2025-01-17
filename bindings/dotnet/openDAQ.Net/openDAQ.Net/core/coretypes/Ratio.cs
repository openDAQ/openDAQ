/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
//     RTGen (CSharpGenerator v1.0.0) on 15.10.2024 16:27:56.
// </auto-generated>
//------------------------------------------------------------------------------


namespace Daq.Core.Types;


[StructLayout(LayoutKind.Sequential)]
internal unsafe class RawRatio : RawBaseObject
{
    //ErrorCode getNumerator(daq.Int* numerator); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out long, ErrorCode> GetNumerator;
    //ErrorCode getDenominator(daq.Int* denominator); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out long, ErrorCode> GetDenominator;
    //ErrorCode simplify(daq.IRatio** simplifiedRatio); stdcall;
    public delegate* unmanaged[Stdcall]<IntPtr, out IntPtr, ErrorCode> Simplify;
}

[Guid("08d28c13-55a6-5fe5-a0f0-19a3f8707c15")]
public class Ratio : BaseObject
{
    //type-casted base._virtualTable
    private readonly RawRatio _rawRatio;

    internal Ratio(IntPtr nativePointer, bool incrementReference)
        : base(nativePointer, incrementReference)
    {
        IntPtr objVirtualTable = Marshal.ReadIntPtr(nativePointer, 0); //read the pointer from the given address
        base._virtualTable =
            _rawRatio = Marshal.PtrToStructure<RawRatio>(objVirtualTable);
    }

    #region properties

    /// <summary>Gets numerator part.</summary>
    public long Numerator
    {
        get
        {
            //native output argument
            long numerator;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawRatio.GetNumerator(base.NativePointer, out numerator);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return numerator;
        }
    }

    /// <summary>Gets denominator part.</summary>
    public long Denominator
    {
        get
        {
            //native output argument
            long denominator;

            unsafe //use native function pointer
            {
                //call native function
                ErrorCode errorCode = (ErrorCode)_rawRatio.GetDenominator(base.NativePointer, out denominator);

                if (Result.Failed(errorCode))
                {
                    throw new OpenDaqException(errorCode);
                }
            }

            return denominator;
        }
    }

    #endregion properties

    /// <summary>Simplifies rational number if possible and returns the simplified ratio as a new object.</summary>
    /// <remarks>
    /// Call this method to reduce stored rational number to the lowest terms possible.
    /// Example: 10/100 is reduced to 1/10.
    /// </remarks>
    /// <returns>the simplified ratio.</returns>
    public Ratio Simplify()
    {
        //native output argument
        IntPtr simplifiedRatioPtr;

        unsafe //use native function pointer
        {
            //call native function
            ErrorCode errorCode = (ErrorCode)_rawRatio.Simplify(base.NativePointer, out simplifiedRatioPtr);

            if (Result.Failed(errorCode))
            {
                throw new OpenDaqException(errorCode);
            }
        }

        // validate pointer
        if (simplifiedRatioPtr == IntPtr.Zero)
        {
            return default;
        }

        return new Ratio(simplifiedRatioPtr, incrementReference: false);
    }

    #region operators

    /// <summary>Performs an implicit conversion from <see cref="Daq.Core.Types.Ratio"/> to <see cref="double"/>.</summary>
    /// <param name="value">The SDK <c>Ratio</c>.</param>
    /// <returns>The managed <c>double</c> value.</returns>
    public static implicit operator double(Ratio value) => (value == null) ? double.NaN : (double)value.Numerator / (double)value.Denominator;

    /// <summary>Determines whether this instance and a specified <c>double</c>, have the same value.</summary>
    /// <param name="other">The other <c>double</c> to compare to this instance.</param>
    /// <returns><c>true</c> if the other <c>double</c> value is the same as this instance; otherwise, <c>false</c>.</returns>
    public bool Equals(double other) => ((double)this).Equals(other);

    /// <summary>Determines whether this instance and a specified <c>Ratio</c>, have the same value.</summary>
    /// <param name="other">The other <c>Ratio</c> to compare to this instance.</param>
    /// <returns><c>true</c> if the other <c>Ratio</c> value is the same as this instance; otherwise, <c>false</c>.</returns>
    public bool Equals(Ratio other) => (this.Numerator == other?.Numerator) && (this.Denominator == other?.Denominator);

    /// <summary>
    /// Implements the operator to multiply two <c>Ratio</c> objects.
    /// </summary>
    /// <param name="x">The first <c>Ratio</c>.</param>
    /// <param name="y">The second <c>Ratio</c>.</param>
    /// <returns>The result of the operator.</returns>
    public static Ratio operator *(Ratio x, Ratio y)
    {
        CheckDenominatorAndThrow(x.Denominator);
        CheckDenominatorAndThrow(y.Denominator);

        return CoreTypesFactory.CreateRatio(x.Numerator * y.Numerator, x.Denominator * y.Denominator);
    }

    /// <summary>
    /// Implements the operator to multiply a long value with a <c>Ratio</c> object's numerator.
    /// </summary>
    /// <param name="x">The <c>long</c> value.</param>
    /// <param name="y">The <c>Ratio</c>.</param>
    /// <returns>The result of the operator.</returns>
    public static Ratio operator *(long x, Ratio y)
    {
        CheckDenominatorAndThrow(y.Denominator);

        return CoreTypesFactory.CreateRatio(x * y.Numerator, y.Denominator);
    }

    /// <summary>
    /// Implements the operator to multiply a <c>Ratio</c> object's numerator with a long value.
    /// </summary>
    /// <param name="y">The <c>Ratio</c>.</param>
    /// <param name="x">The <c>long</c> value.</param>
    /// <returns>The result of the operator.</returns>
    public static Ratio operator *(Ratio y, long x)
    {
        return x * y;
    }

    /// <summary>
    /// Implements the operator to divide a long value by a <c>Ratio</c> object.
    /// </summary>
    /// <param name="x">The <c>long</c> value.</param>
    /// <param name="y">The <c>Ratio</c>.</param>
    /// <returns>The result of the operator.</returns>
    public static Ratio operator /(long x, Ratio y)
    {
        CheckDenominatorAndThrow(y.Numerator);

        return CoreTypesFactory.CreateRatio(x * y.Denominator, y.Numerator);
    }

    /// <summary>
    /// Implements the operator to divide a <c>Ratio</c> object by a long value.
    /// </summary>
    /// <param name="y">The <c>Ratio</c>.</param>
    /// <param name="x">The <c>long</c> value.</param>
    /// <returns>The result of the operator.</returns>
    public static Ratio operator /(Ratio y, long x)
    {
        CheckDenominatorAndThrow(y.Denominator);
        CheckDenominatorAndThrow(x);

        return CoreTypesFactory.CreateRatio(y.Numerator, y.Denominator * x);
    }

    /// <summary>
    /// Implements the operator to divide two <c>Ratio</c> objects.
    /// </summary>
    /// <param name="x">The first <c>Ratio</c>.</param>
    /// <param name="y">The second <c>Ratio</c>.</param>
    /// <returns>The result of the operator.</returns>
    public static Ratio operator /(Ratio x, Ratio y)
    {
        CheckDenominatorAndThrow(x.Denominator);
        CheckDenominatorAndThrow(y.Numerator);

        return CoreTypesFactory.CreateRatio(x.Numerator * y.Denominator, x.Denominator * y.Numerator);
    }

    private static void CheckDenominatorAndThrow(long denominator)
    {
        if (denominator == 0)
            throw new ArgumentOutOfRangeException(nameof(denominator), "Denominator can't be zero");
    }

    #endregion operators
}


#region Class Factory

// Factory functions of the &apos;CoreTypes&apos; library.
public static partial class CoreTypesFactory
{
    //ErrorCode createRatio(daq.IRatio** obj, daq.Int numerator, daq.Int denominator); cdecl;
    [DllImport(CoreTypesDllInfo.FileName, CallingConvention = CallingConvention.Cdecl)]
    private static extern ErrorCode createRatio(out IntPtr obj, long numerator, long denominator);

    public static ErrorCode CreateRatio(out Ratio obj, long numerator, long denominator)
    {
        //initialize output argument
        obj = default;

        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createRatio(out objPtr, numerator, denominator);

        if (Result.Succeeded(errorCode))
        {
            //create object
            obj = new Ratio(objPtr, incrementReference: false);
        }

        return errorCode;
    }

    public static Ratio CreateRatio(long numerator, long denominator)
    {
        //native output argument
        IntPtr objPtr;

        //call native function
        ErrorCode errorCode = createRatio(out objPtr, numerator, denominator);

        if (Result.Failed(errorCode))
        {
            throw new OpenDaqException(errorCode);
        }

        //create and return object
        return new Ratio(objPtr, incrementReference: false);
    }
}

#endregion Class Factory
