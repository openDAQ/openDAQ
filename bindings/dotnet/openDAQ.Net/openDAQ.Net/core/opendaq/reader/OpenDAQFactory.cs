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


using Daq.Core.Types;
using Daq.Core.Objects;


namespace Daq.Core.OpenDAQ;


// Factory functions of the &apos;OpenDAQ&apos; library.
public static partial class OpenDAQFactory
{
#warning Building manually extended factory for SampleReaders.

    /// <summary>
    /// Gets the <see cref="SampleType"/> for the given <typeparamref name="TValueType"/>.
    /// </summary>
    /// <typeparam name="TValueType">The value type to convert to the <see cref="SampleType"/> enumeration.</typeparam>
    /// <returns>The <see cref="SampleType"/>.</returns>
    public static SampleType GetSampleType<TValueType>()
        where TValueType : struct
    {
        //create a .NET variable to be switched for its type
        TValueType value = default;

        switch (value) //discarding all casts since we do not need the value
        {
          //case ? _:      return SampleType.Int8;
            case Byte _:   return SampleType.UInt8;
            case Int16 _:  return SampleType.Int16;
            case UInt16 _: return SampleType.UInt16;
            case Int32 _:  return SampleType.Int32;
            case UInt32 _: return SampleType.UInt32;
            case Int64 _:  return SampleType.Int64;
          //case ? _:      return SampleType.RangeInt64; //ToDo: a generic struct in openDAQ with operators and `{ T start; T end; }` as fields
            case UInt64 _: return SampleType.UInt64;
            case Single _: return SampleType.Float32;
            case Double _: return SampleType.Float64;
          //case ? _:      return SampleType.ComplexFloat32;
          //case ? _:      return SampleType.ComplexFloat64;
            default:       return SampleType.Undefined;
        }
    }

    /// <summary>
    /// Checks the type of the elements against the given type.
    /// </summary>
    /// <typeparam name="TElementType">The element type.</typeparam>
    /// <param name="otherSampleType">The type to check against.</param>
    /// <param name="errorMsg">The error message in case of an error.</param>
    /// <returns><c>true</c> if the types match, otherwise <c>false</c>.</returns>
    internal static bool CheckSampleType<TElementType>(SampleType otherSampleType, out string errorMsg)
        where TElementType : struct
    {
        errorMsg = null;

        SampleType genericSampleType = OpenDAQFactory.GetSampleType<TElementType>();

        if (genericSampleType == otherSampleType)
            return true;

        errorMsg = $"{genericSampleType} \x2260 {otherSampleType}"; //'!='

        return false;
    }

    #region StreamReader

    public static ErrorCode CreateStreamReader(out StreamReader<double, Int64> obj, Signal signal, ReadMode mode = ReadMode.Scaled, ReadTimeoutType timeoutType = ReadTimeoutType.All)
    {
        return CreateStreamReader<double>(out obj, signal, mode, timeoutType);
    }

    public static ErrorCode CreateStreamReader<TValueType>(out StreamReader<TValueType, Int64> obj, Signal signal, ReadMode mode = ReadMode.Scaled, ReadTimeoutType timeoutType = ReadTimeoutType.All)
        where TValueType : struct
    {
        return CreateStreamReader<TValueType, Int64>(out obj, signal, mode, timeoutType);
    }

    public static StreamReader<double, Int64> CreateStreamReader(Signal signal, ReadMode mode = ReadMode.Scaled, ReadTimeoutType timeoutType = ReadTimeoutType.All)
    {
        return CreateStreamReader<double, Int64>(signal, mode, timeoutType);
    }

    public static StreamReader<TValueType, Int64> CreateStreamReader<TValueType>(Signal signal, ReadMode mode = ReadMode.Scaled, ReadTimeoutType timeoutType = ReadTimeoutType.All)
        where TValueType : struct
    {
        return CreateStreamReader<TValueType, Int64>(signal, mode, timeoutType);
    }

    #endregion StreamReader

    #region MultiReader

    public static ErrorCode CreateMultiReader(out MultiReader<double, Int64> obj, IListObject<Signal> signals, ReadMode mode = ReadMode.Scaled, ReadTimeoutType timeoutType = ReadTimeoutType.All)
    {
        return CreateMultiReader<double>(out obj, signals, mode, timeoutType);
    }

    public static ErrorCode CreateMultiReader<TValueType>(out MultiReader<TValueType, Int64> obj, IListObject<Signal> signals, ReadMode mode = ReadMode.Scaled, ReadTimeoutType timeoutType = ReadTimeoutType.All)
        where TValueType : struct
    {
        return CreateMultiReader<TValueType, Int64>(out obj, signals, mode, timeoutType);
    }

    public static MultiReader<double, Int64> CreateMultiReader(IListObject<Signal> signals, ReadMode mode = ReadMode.Scaled, ReadTimeoutType timeoutType = ReadTimeoutType.All)
    {
        return CreateMultiReader<double, Int64>(signals, mode, timeoutType);
    }

    public static MultiReader<TValueType, Int64> CreateMultiReader<TValueType>(IListObject<Signal> signals, ReadMode mode = ReadMode.Scaled, ReadTimeoutType timeoutType = ReadTimeoutType.All)
        where TValueType : struct
    {
        return CreateMultiReader<TValueType, Int64>(signals, mode, timeoutType);
    }

    #endregion MultiReader

    #region BlockReader

    public static ErrorCode CreateBlockReader(out BlockReader<double, Int64> obj, Signal signal, nuint blockSize, ReadMode mode = ReadMode.Scaled)
    {
        return CreateBlockReader<double>(out obj, signal, blockSize, mode);
    }

    public static ErrorCode CreateBlockReader<TValueType>(out BlockReader<TValueType, Int64> obj, Signal signal, nuint blockSize, ReadMode mode = ReadMode.Scaled)
        where TValueType : struct
    {
        return CreateBlockReader<TValueType, Int64>(out obj, signal, blockSize, mode);
    }

    public static BlockReader<double, Int64> CreateBlockReader(Signal signal, nuint blockSize, ReadMode mode = ReadMode.Scaled)
    {
        return CreateBlockReader<double, Int64>(signal, blockSize, mode);
    }

    public static BlockReader<TValueType, Int64> CreateBlockReader<TValueType>(Signal signal, nuint blockSize, ReadMode mode = ReadMode.Scaled)
        where TValueType : struct
    {
        return CreateBlockReader<TValueType, Int64>(signal, blockSize, mode);
    }

    #endregion BlockReader

    #region TailReader

    public static ErrorCode CreateTailReader(out TailReader<double, Int64> obj, Signal signal, nuint historySize, ReadMode mode = ReadMode.Scaled)
    {
        return CreateTailReader<double>(out obj, signal, historySize, mode);
    }

    public static ErrorCode CreateTailReader<TValueType>(out TailReader<TValueType, Int64> obj, Signal signal, nuint historySize, ReadMode mode = ReadMode.Scaled)
        where TValueType : struct
    {
        return CreateTailReader<TValueType, Int64>(out obj, signal, historySize, mode);
    }

    public static TailReader<double, Int64> CreateTailReader(Signal signal, nuint historySize, ReadMode mode = ReadMode.Scaled)
    {
        return CreateTailReader<double, Int64>(signal, historySize, mode);
    }

    public static TailReader<TValueType, Int64> CreateTailReader<TValueType>(Signal signal, nuint historySize, ReadMode mode = ReadMode.Scaled)
        where TValueType : struct
    {
        return CreateTailReader<TValueType, Int64>(signal, historySize, mode);
    }

    #endregion TailReader
}
