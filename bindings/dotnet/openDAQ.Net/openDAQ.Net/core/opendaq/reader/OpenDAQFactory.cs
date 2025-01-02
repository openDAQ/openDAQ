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


using Daq.Core.Types;
using Daq.Core.Objects;
using System.Numerics;


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

        return value switch //discarding all casts since we do not need the value
        {
          // ?     _ => SampleType.Int8;
            Byte   _ => SampleType.UInt8,
            Int16  _ => SampleType.Int16,
            UInt16 _ => SampleType.UInt16,
            Int32  _ => SampleType.Int32,
            UInt32 _ => SampleType.UInt32,
            Int64  _ => SampleType.Int64,
          // ?     _ => SampleType.RangeInt64; //ToDo: a generic struct in openDAQ with operators and `{ T start; T end; }` as fields
            UInt64 _ => SampleType.UInt64,
            Single _ => SampleType.Float32,
            Double _ => SampleType.Float64,
          // ?     _ => SampleType.ComplexFloat32;
          // ?     _ => SampleType.ComplexFloat64;
                   _ => SampleType.Undefined,
        };
    }

    /// <summary>
    /// Gets the .NET type for the given <see cref="SampleType"/>.
    /// </summary>
    /// <param name="sampleType">The <see cref="SampleType"/> to convert to a .NET type.</param>
    /// <returns>The .NET type.</returns>
    public static Type GetSampleType(SampleType sampleType)
    {
        return sampleType switch
        {
            SampleType.Float32        => typeof(float),
            SampleType.Float64        => typeof(double),
            SampleType.UInt8          => typeof(byte),
            SampleType.Int8           => typeof(sbyte),
            SampleType.UInt16         => typeof(UInt16),
            SampleType.Int16          => typeof(Int16),
            SampleType.UInt32         => typeof(UInt32),
            SampleType.Int32          => typeof(Int32),
            SampleType.UInt64         => typeof(UInt64),
            SampleType.Int64          => typeof(Int64),
            SampleType.RangeInt64     => null,
            SampleType.ComplexFloat32 => null,
            SampleType.ComplexFloat64 => null,
            SampleType.Binary         => null,
            SampleType.String         => typeof(string),
            SampleType.Struct         => null,
          //SampleType.Invalid        =>
          //SampleType.Undefined      =>
          //SampleType._count         =>
            _                         => null
        };
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

    //ToDo: no `dynamic` for now (unwanted)
    //      This syntax results in a dynamic object as its type is determined at runtime.
    //      The compiler cannot check any calls on this object so in case of a syntax error the code will be compiled but fail at runtime.
    //
    //public static dynamic CreateTailReader(Signal signal, nuint historySize, SampleType valueType, SampleType domainType, ReadMode mode = ReadMode.Scaled)
    //{
    //    var functions = typeof(OpenDAQFactory).GetMethods(System.Reflection.BindingFlags.Public | System.Reflection.BindingFlags.Static);
    //    var createFunction = functions.Where(mi =>    mi.Name.Equals(nameof(CreateTailReader))
    //                                               && mi.ReturnType.Name.Equals(typeof(TailReader<,>).Name)
    //                                               && (mi.GetGenericArguments().Length == 2))
    //                                  .FirstOrDefault();

    //    dynamic tailReader = createFunction?.MakeGenericMethod(GetSampleType(valueType), GetSampleType(domainType))
    //                                        .Invoke(null, new object[] { signal, historySize, mode });
    //    return tailReader;
    //}
}
