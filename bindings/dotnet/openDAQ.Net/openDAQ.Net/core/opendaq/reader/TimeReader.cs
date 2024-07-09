/*
 * Copyright 2022-2024 openDAQ d. o. o.
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


namespace Daq.Core.OpenDAQ;


/// <summary>A reader wrapper that can convert domain clock-ticks to time stamps when the domain is time.</summary>
//[Guid("4D44E982-7658-4B79-8B6B-F3D918E18596")] //manually created
public class TimeReader
{
    private const  long TICKS_PER_MICROSECOND  = 10L; //from DateTime struct
    private const  long TICKS_PER_SECOND       = TICKS_PER_MICROSECOND * 1000000L;
    //private static long TICKS_PER_SECOND_POWER = (long)Math.Log10(TICKS_PER_SECOND);

    private const string BLOCK_READER_NAME           = nameof(BlockReader<double, Int64>);
    private const string MULTI_READER_NAME           = nameof(MultiReader<double, Int64>);
    private const string PACKET_READER_NAME          = nameof(PacketReader);
    private const string STEAM_READER_NAME           = nameof(StreamReader<double, Int64>);
    private const string TAIL_READER_NAME            = nameof(TailReader<double, Int64>);
    private const string BLOCK_READER_BLOCKSIZE_NAME = nameof(BlockReader<double, Int64>.BlockSize);

    private readonly SampleReader                       _reader;
    private readonly (long Numerator, long Denominator) _resolution;
    private readonly DateTimeOffset                     _origin;
    private readonly string                             _unitSymbol;
    private readonly nuint                              _blockSize; //BlockReader only
    private readonly double                             _secondsToTicksFactorFloat;

    private TimeReader()
    {
        //block direct instantiation, use Class Factory at the bottom of this file
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="TimeReader"/> struct.
    /// </summary>
    /// <param name="reader">The sample reader.</param>
    /// <param name="domainDescriptor">The <c>DataDescriptor</c> of the domain signal.</param>
    /// <remarks>Internal. Use Class Factory at the bottom of this file to instantiate a TimeReader.</remarks>
    internal TimeReader(SampleReader reader, DataDescriptor domainDescriptor)
    {
        _reader = reader;

        using Ratio resolution = domainDescriptor.TickResolution;

        _resolution = (resolution.Numerator, resolution.Denominator);
        _unitSymbol = domainDescriptor.Unit.Symbol;

        //https://learn.microsoft.com/en-us/dotnet/standard/datetime/converting-between-time-zones
        //https://learn.microsoft.com/en-us/dotnet/standard/datetime/access-utc-and-local

        //get the origin date and time (fall-back when not possible to parse) -> LocalDateTime has then e.g. MEZ with an offset +01:00:00
        if (!DateTimeOffset.TryParse(domainDescriptor.Origin, out _origin))
            _origin = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);

        //for later timestamp-arithmetics set local offset to "now" to get daylight-saving-offset -> LocalDateTime has then e.g. MESZ with an offset +02:00:00
        //possible issue: when DLST changes during a measurement the timestamps will simply continue with the previous offset
        _origin = _origin.ToOffset(TimeZoneInfo.Local.GetUtcOffset(DateTimeOffset.Now));

        if (domainDescriptor.Unit.Quantity != "time")
            Console.Error.WriteLine("*** domain signal is no time signal"); //ToDo: handle non-time domain

        if (_unitSymbol != "s")
            Console.Error.WriteLine("*** domain signal unit not in seconds"); //ToDo: handle non-seconds domain

        //need to get block-size from BlockReader; use 1 for other readers
        if ((reader.GetType() is Type readerType)
            && readerType.Name.Equals(BLOCK_READER_NAME + "`2")) //"BlockReader`2" <-> "BlockReader"
        {
            //use reflection to get the block-size
            System.Reflection.PropertyInfo blockSizeGetter = readerType.GetProperty(BLOCK_READER_BLOCKSIZE_NAME);
            if (blockSizeGetter == null)
            {
                //should never come here
                Console.Error.WriteLine($"No property {BLOCK_READER_NAME}<TValue,TDomain>.{BLOCK_READER_BLOCKSIZE_NAME} found!");
            }
            else
            {
                _blockSize = (nuint)blockSizeGetter.GetValue(reader);
            }
        }
        else
        {
            _blockSize = 1;
        }

        _secondsToTicksFactorFloat = 0D; //if it is 0, the raw value already contains ticks, no calculation necessary

        if ((_resolution.Numerator != 1) || (_resolution.Denominator != TICKS_PER_SECOND))
        {
            //get the factor for ticks from seconds calculation (_resolution calculates raw value to seconds)
            _secondsToTicksFactorFloat = (double)TICKS_PER_SECOND * _resolution.Numerator / _resolution.Denominator;
        }
    }

    /// <summary>
    /// Determines whether the specified reader's type is supported.
    /// </summary>
    /// <param name="reader">The reader.</param>
    /// <returns><c>true</c> if reader type is supported; otherwise, <c>false</c>.</returns>
    internal static bool IsReaderTypeSupported(SampleReader reader)
    {
        switch (reader.GetType().Name.Split('`')[0]) //e.g. "BlockReader`2" -> "BlockReader"
        {
            case BLOCK_READER_NAME:
          //case MULTI_READER_NAME:
          //case PACKET_READER_NAME:
            case STEAM_READER_NAME:
            case TAIL_READER_NAME:
                return true;

            default:
                return false;
        }
    }

    /// <summary>
    /// Depending on the reader type, copies at maximum the next or last <c>count</c> unread samples and time stamps (UTC) to the <c>samples</c> and <c>domain</c> buffers.<br/>
    /// The amount actually read is returned through the <c>count</c> parameter.<br/>
    /// It also depends on the reader type how samples are being read.
    /// </summary>
    /// <param name="samples">
    /// The buffer that the samples will be copied to.<br/>
    /// The buffer must be a contiguous memory big enough to receive <c>count</c> amount of samples.
    /// </param>
    /// <param name="timeStamps">
    /// The buffer that the time stamps will be copied to.<br/>
    /// The buffer must be big enough to receive <c>count</c> amount of time stamps.
    /// </param>
    /// <param name="count">
    /// The maximum amount of samples to be read.<br/>
    /// If <c>count</c> is less than available the parameter value is set to the actual amount and only the available
    /// samples are returned. The rest of the buffer is not modified or cleared.
    /// </param>
    /// <param name="timeoutMs">
    /// The maximum amount of time in milliseconds to wait for the requested amount of samples before returning
    /// (optional, and not available for all reader types).
    /// </param>
    /// <returns>
    /// The status of the reader.<br/>
    /// - If the reader is invalid, IReaderStatus::getValid returns false.<br/>
    /// - If an event packet was encountered during processing, IReaderStatus::getReadStatus returns ReadStatus::Event<br/>
    /// - If the reading process is successful, IReaderStatus::getReadStatus returns ReadStatus::Ok, indicating that IReaderStatus::getValid is true and there is no encountered events
    /// </returns>
    /// <exception cref="Daq.Core.Types.OpenDaqException">
    /// <see cref="ErrorCode.OPENDAQ_ERR_SIZETOOSMALL"/> - TimeReader error: The <c>samples</c> and <c>timeStamps</c> arrays have to have a minimum length of <c>count</c>.<br/>
    /// <see cref="ErrorCode.OPENDAQ_ERR_NOT_SUPPORTED"/> - TimeReader error: Reader type '&lt;type-name&gt;' not supported.
    /// </exception>
    public ReaderStatus ReadWithDomain<TValue>(TValue[] samples, DateTime[] timeStamps, ref nuint count, nuint timeoutMs = 0)
        where TValue : struct
    {
        if (samples.Length < (int)count)
        {
            throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_SIZETOOSMALL,
                                       $"TimeReader error: The {nameof(samples)} array has to have a minimum length of {nameof(count)} ({samples.Length} < {count}).");
        }

        if (timeStamps.Length < (int)count)
        {
            throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_SIZETOOSMALL,
                                       $"TimeReader error: The {nameof(timeStamps)} array has to have a minimum length of {nameof(count)} ({timeStamps.Length} < {count}).");
        }

        switch (_reader.DomainReadType)
        {
            case SampleType.Float32:            return ReadWithDomainByReaderType<TValue, float>(samples, ref count, timeoutMs, timeStamps);
            case SampleType.Float64:            return ReadWithDomainByReaderType<TValue, double>(samples, ref count, timeoutMs, timeStamps);
            case SampleType.UInt8:              return ReadWithDomainByReaderType<TValue, byte>(samples, ref count, timeoutMs, timeStamps);
            case SampleType.Int8:               return ReadWithDomainByReaderType<TValue, sbyte>(samples, ref count, timeoutMs, timeStamps);
            case SampleType.UInt16:             return ReadWithDomainByReaderType<TValue, UInt16>(samples, ref count, timeoutMs, timeStamps);
            case SampleType.Int16:              return ReadWithDomainByReaderType<TValue, Int16>(samples, ref count, timeoutMs, timeStamps);
            case SampleType.UInt32:             return ReadWithDomainByReaderType<TValue, UInt32>(samples, ref count, timeoutMs, timeStamps);
            case SampleType.Int32:              return ReadWithDomainByReaderType<TValue, Int32>(samples, ref count, timeoutMs, timeStamps);
            case SampleType.UInt64:             return ReadWithDomainByReaderType<TValue, UInt64>(samples, ref count, timeoutMs, timeStamps);
            case SampleType.Int64:              return ReadWithDomainByReaderType<TValue, Int64>(samples, ref count, timeoutMs, timeStamps);

            //case SampleType.RangeInt64:
            //    break;
            //case SampleType.ComplexFloat32:
            //    break;
            //case SampleType.ComplexFloat64:
            //    break;
            //case SampleType.Binary:
            //    break;
            //case SampleType.String:
            //    break;
            //case SampleType.Struct:
            //    break;

            default:
                count = 0;
                return null;
        }
    }

    /// <summary>
    /// Reads the samples and domain values using the injected reader and transforms the domain values to time stamps.
    /// </summary>
    /// <typeparam name="TValue">The type of the samples.</typeparam>
    /// <typeparam name="TDomain">The type of the domain values.</typeparam>
    /// <param name="samples">The samples array.</param>
    /// <param name="count">In: The maximum number of samples to read. Out: The number of samples actually read.</param>
    /// <param name="timeoutMs">The timeout in ms, ignored by some readers.</param>
    /// <param name="timeStamps">The resulting time stamps array (should have the same size as the <c>samples</c> array, but minimum a length of <c>count</c>).</param>
    /// <returns>The <c>ReaderStatus</c>.</returns>
    /// <exception cref="Daq.Core.Types.OpenDaqException"><see cref="ErrorCode.OPENDAQ_ERR_NOT_SUPPORTED"/> - TimeReader error: Reader type '&lt;type-name&gt;' not supported.</exception>
    private ReaderStatus ReadWithDomainByReaderType<TValue, TDomain>(TValue[] samples, ref nuint count, nuint timeoutMs, DateTime[] timeStamps)
        where TValue : struct
        where TDomain : struct
    {
        // get a buffer for the domain values
        var       domainPool = System.Buffers.ArrayPool<TDomain>.Shared;
        TDomain[] domain     = domainPool.Rent(samples.Length);

        // Result
        ReaderStatus status;

        try
        {
            switch (_reader)
            {
                case BlockReader<TValue, TDomain> blockReader:
                    status = blockReader.ReadWithDomain(samples, domain, ref count, timeoutMs);
                    break;

                case MultiReader<TValue, TDomain> multiReader:
                    status = multiReader.ReadWithDomain(samples, domain, ref count, timeoutMs);
                    break;

                //case PacketReader packetReader: // not supported

                case StreamReader<TValue, TDomain> streamReader:
                    status = streamReader.ReadWithDomain(samples, domain, ref count, timeoutMs);
                    break;

                case TailReader<TValue, TDomain> tailReader:
                    status = tailReader.ReadWithDomain(samples, domain, ref count);
                    break;

                default: //should never come here due to check in factory below
                    throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_NOT_SUPPORTED, $"TimeReader error: Reader type '{_reader.GetType().Name}' not supported");
            }

            if (status?.ReadStatus == ReadStatus.Ok)
            {
                Transform(domain, timeStamps, sampleCount: count * _blockSize); //_blockSize is only relevant for BlockReader (see constructor)
            }
            else
            {
                Console.Error.WriteLine($"ReadWithDomain error: {status?.ReadStatus}");
            }
        }
        finally
        {
            // return the buffer for the domain values
            domainPool.Return(domain);
        }

        return status;
    }

    /// <summary>
    /// Transforms the domain values to time stamps.
    /// </summary>
    /// <typeparam name="TDomain">The type of the domain values.</typeparam>
    /// <param name="domainValues">The domain values array.</param>
    /// <param name="timeStamps">The time stamps array.</param>
    /// <param name="sampleCount">The number of values to transform.</param>
    private void Transform<TDomain>(TDomain[] domainValues, DateTime[] timeStamps, nuint sampleCount)
        where TDomain : struct
    {
        //transform domain values to DateTime
        //use AddTicks() to get the full microseconds resolution (should be even 100ns) as all other Add functions always round full integer number, cutting fractions away
        //AddMicroseconds() is only available starting .NET 8

        if (_secondsToTicksFactorFloat != 0D)
        {
            //calculate (rounded) ticks
            for (nuint index = 0; index < sampleCount; ++index)
            {
                long ticks = (long)(Convert.ToDouble(domainValues[index]) * _secondsToTicksFactorFloat + 0.5D); //conversion needed due to generic array
                timeStamps[index] = _origin.AddTicks(ticks).LocalDateTime;
            }
        }
        else
        {
            //raw values are already ticks (100 nanoseconds resolution)
            for (nuint index = 0; index < sampleCount; ++index)
            {
                long ticks = Convert.ToInt64(domainValues[index]); //conversion needed due to generic array
                timeStamps[index] = _origin.AddTicks(ticks).LocalDateTime;
            }
        }
    }
}


#region Class Factory

// Factory functions of the &apos;OpenDAQ&apos; library.
public static partial class OpenDAQFactory
{
    /// <summary>
    /// Creates the time reader.
    /// </summary>
    /// <param name="reader">The <see cref="SampleReader"/> to read the data from.</param>
    /// <param name="signal">The signal with a <see cref="Signal.DomainSignal"/> or domain-signal itself, for the domain configuration.</param>
    /// <returns>The <c>TimeReader</c> instance, when the domain signal is a time signal; otherwise <c>null</c>.</returns>
    public static TimeReader CreateTimeReader(SampleReader reader, Signal signal)
    {
        if (!TimeReader.IsReaderTypeSupported(reader))
            throw new OpenDaqException(ErrorCode.OPENDAQ_ERR_NOT_SUPPORTED, $"CreateTimeReader(): Reader type '{reader.GetType().Name}' not supported");

        //when signal.DomainSignal==null, signal is most probably already the domain signal
        Signal domainSignal = signal.DomainSignal ?? signal;

        using Objects.Unit unit = domainSignal.Descriptor.Unit;

        if (unit.Quantity != "time")
        {
            Console.Error.WriteLine("*** domain signal is no time signal");
            return null;
        }

        if (unit.Symbol != "s")
            Console.Error.WriteLine("*** domain signal unit not in seconds");

        return new TimeReader(reader, domainSignal.Descriptor);
    }
}

#endregion Class Factory
