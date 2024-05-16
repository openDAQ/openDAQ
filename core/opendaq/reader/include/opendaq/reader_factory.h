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
#pragma once
#include <opendaq/stream_reader_ptr.h>
#include <opendaq/block_reader_ptr.h>
#include <opendaq/tail_reader_ptr.h>
#include <opendaq/packet_reader_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/input_port_ptr.h>
#include <opendaq/multi_reader_ptr.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/signal_ptr.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/reader_status_ptr.h>
#include <opendaq/block_reader_status_ptr.h>
#include <opendaq/multi_reader_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

using UndefinedType = void;

inline ReaderStatusPtr ReaderStatus(const EventPacketPtr& packet = nullptr, Bool valid = true, const StringPtr& errorMessage = nullptr)
{
    return ReaderStatus_Create(packet, valid, errorMessage);
}

inline BlockReaderStatusPtr BlockReaderStatus(
    const EventPacketPtr& packet = nullptr, 
    Bool valid = true,  
    SizeT readSamples = 0,
    const StringPtr& errorMessage = nullptr)
{
    return BlockReaderStatus_Create(packet, valid, readSamples, errorMessage);
}

/*!
 * @brief Creates a reader that eases reading packets from the signal.
 * @param signal The signal to read the packets from.
 */
inline PacketReaderPtr PacketReader(SignalPtr signal)
{
    return PacketReader_Create(signal);
}

/*!
 * @brief Creates a reader that eases reading packets from the port.
 * @param input The input port to read the packets from.
 */
inline PacketReaderPtr PacketReaderFromPort(InputPortConfigPtr port)
{
    return PacketReaderFromPort_Create(port);
}

/*!
 * @brief Creates a signal data reader that abstracts away reading of signal packets by keeping an
 * internal read-position and automatically advances it on subsequent reads.
 * @param port The port to read the data from.
 * @param valueReadType The sample-type type to read signal values as. Implicitly convert from actual type to this one if conversion exists.
 * @param domainReadType The sample-type type to read signal domain as. Implicitly convert from actual type to this one if conversion exists.
 * @param timeoutType The type of time-out to use. See @see ReadTimeoutType.
 */
inline StreamReaderPtr StreamReaderFromPort(InputPortConfigPtr port,
                                    SampleType valueReadType,
                                    SampleType domainReadType,
                                    ReadMode mode = ReadMode::Scaled,
                                    ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return StreamReaderFromPort_Create(port, valueReadType, domainReadType, mode, timeoutType);
}

/*!
 * @brief Creates a signal data reader that abstracts away reading of signal packets by keeping an
 * internal read-position and automatically advances it on subsequent reads.
 * @param signal The signal to read the data from.
 * @param valueReadType The sample-type type to read signal values as. Implicitly convert from actual type to this one if conversion exists.
 * @param domainReadType The sample-type type to read signal domain as. Implicitly convert from actual type to this one if conversion exists.
 * @param timeoutType The type of time-out to use. See @see ReadTimeoutType.
 */
inline StreamReaderPtr StreamReader(SignalPtr signal,
                                    SampleType valueReadType,
                                    SampleType domainReadType,
                                    ReadMode mode = ReadMode::Scaled,
                                    ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return StreamReader_Create(signal, valueReadType, domainReadType, mode, timeoutType);
}

/*!
 * @brief Creates a signal data reader that abstracts away reading of signal packets by keeping an
 * internal read-position and automatically advances it on subsequent reads.
 * @param signal The signal to read the data from.
 * @param timeoutType The type of time-out to use. See @see ReadTimeoutType.
 * @tparam TValueType The sample-type type to read signal values as. Implicitly convert from actual type to
 * this one if conversion exists.
 * @tparam TDomainType The sample-type type to read signal domain as. Implicitly convert from actual type to
 * this one if conversion exists.
 */
template <typename TValueType = double, typename TDomainType = ClockTick>
StreamReaderPtr StreamReader(SignalPtr signal, ReadMode mode, ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return StreamReader(
        signal,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType,
        mode,
        timeoutType
    );
}

/*!
 * @brief Creates a signal data reader that abstracts away reading of signal packets by keeping an
 * internal read-position and automatically advances it on subsequent reads.
 * @param signal The signal to read the data from.
 * @param timeoutType The type of time-out to use. See @see ReadTimeoutType.
 * @tparam TValueType The sample-type type to read signal values as. Implicitly convert from actual type to
 * this one if conversion exists.
 * @tparam TDomainType The sample-type type to read signal domain as. Implicitly convert from actual type to
 * this one if conversion exists.
 */
template <typename TValueType = double, typename TDomainType = ClockTick>
StreamReaderPtr StreamReader(SignalPtr signal, ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return StreamReader(
        signal,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType,
        ReadMode::Scaled,
        timeoutType
    );
}

/*!
 * @brief Creates a signal data reader that abstracts away reading of signal packets by keeping an
 * internal read-position and automatically advances it on subsequent reads.
 * @param signal The signal to read the data from.
 * @param timeoutType The type of time-out to use. See @see ReadTimeoutType.
 */
inline StreamReaderPtr StreamReader(SignalPtr signal, ReadMode mode = ReadMode::Scaled, ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return StreamReader<>(signal, mode, timeoutType);
}

/*!
 * @brief Creates a new reader using the data of the existing one.
 * Used when a StreamReader gets invalidated because of incompatible change in signal descriptor.
 * This will reuse all of the read and connection info so no data is lost when changing to the new reader type.
 * @param invalidatedReader The reader from which to steal the read and connection info. If the passed-in
 * reader is not invalidated it will be invalidated after the call.
 * @param valueReadType The sample-type type to read signal values as. Implicitly convert from actual type to
 * this one if conversion exists.
 * @param domainReadType The sample-type type to read signal domain as. Implicitly convert from actual type to
 * this one if conversion exists.
 */
inline StreamReaderPtr StreamReaderFromExisting(StreamReaderPtr invalidatedReader, SampleType valueReadType, SampleType domainReadType)
{
    return StreamReaderFromExisting_Create(invalidatedReader, valueReadType, domainReadType);
}

/*!
 * @brief Creates a new reader using the data of the existing one.
 * Used when a StreamReader gets invalidated because of incompatible change in signal descriptor.
 * This will reuse all of the read and connection info so no data is lost when changing to the new reader type.
 * @param invalidatedReader The reader from which to steal the read and connection info. If the passed-in
 * reader is not invalidated it will be invalidated after the call.
 * @tparam TValueType The sample-type type to read signal values as. Implicitly convert from actual type to
 * this one if conversion exists.
 * @tparam TDomainType The sample-type type to read signal domain as. Implicitly convert from actual type to
 * this one if conversion exists.
 */
template <typename TValueType = double, typename TDomainType = ClockTick>
StreamReaderPtr StreamReaderFromExisting(StreamReaderPtr invalidatedReader)
{
    return StreamReaderFromExisting(
        invalidatedReader,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType
    );
}

/*!
 * @brief A reader that only ever reads the last N samples, subsequent calls may result in overlapping data.
 * @param signal The signal to read the data from.
 * @param historySize The maximum amount of samples in history to keep.
 * @param valueReadType The sample-type type to read signal values as. Implicitly convert from actual type to
 * this one if conversion exists.
 * @param domainReadType The sample-type type to read signal domain as. Implicitly convert from actual type to
 * this one if conversion exists.
 */
inline TailReaderPtr TailReader(SignalPtr signal,
                                SizeT historySize,
                                SampleType valueReadType,
                                SampleType domainReadType,
                                ReadMode mode = ReadMode::Scaled)
{
    return TailReader_Create(signal, historySize, valueReadType, domainReadType, mode);
}
/*!
 * @brief A reader that only ever reads the last N samples, subsequent calls may result in overlapping data.
 * @param port The port to read the data from.
 * @param historySize The maximum amount of samples in history to keep.
 * @param valueReadType The sample-type type to read signal values as. Implicitly convert from actual type to
 * this one if conversion exists.
 * @param domainReadType The sample-type type to read signal domain as. Implicitly convert from actual type to
 * this one if conversion exists.
 */
inline TailReaderPtr TailReaderFromPort(InputPortConfigPtr port,
                                        SizeT historySize,
                                        SampleType valueReadType,
                                        SampleType domainReadType,
                                        ReadMode mode = ReadMode::Scaled)
{
    return TailReaderFromPort_Create(port, historySize, valueReadType, domainReadType, mode);
}


/*!
 * @brief A reader that only ever reads the last N samples, subsequent calls may result in overlapping data.
 * @param signal The signal to read the data from.
 * @param historySize The maximum amount of samples in history to keep.
 * @tparam TValueType The sample-type type to read signal values as. Implicitly convert from actual type to
 * this one if conversion exists.
 * @tparam TDomainType The sample-type type to read signal domain as. Implicitly convert from actual type to
 * this one if conversion exists.
 */
template <typename TValueType = double, typename TDomainType = ClockTick>
TailReaderPtr TailReader(SignalPtr signal, SizeT historySize, ReadMode mode = ReadMode::Scaled)
{
    return TailReader(
        signal,
        historySize,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType,
        mode
    );
}

/*!
 * @brief A reader that only ever reads the last N samples, subsequent calls may result in overlapping data.
 * @param signal The signal to read the data from.
 * @param historySize The maximum amount of samples in history to keep.
 */
inline TailReaderPtr TailReader(SignalPtr signal, SizeT historySize, ReadMode mode = ReadMode::Scaled)
{
    return TailReader<>(std::move(signal), historySize, mode);
}

/*!
 * @brief Creates a new reader using the data of the existing one.
 * Used when a TailReader gets invalidated because of incompatible change in the signal descriptor.
 * This will reuse all of the read and connection info so no data is lost when changing to the new reader type.
 * @param invalidatedReader The reader from which to steal the read and connection info. If the passed-in
 * reader is not invalidated it will be invalidated after the call.
 * @param historySize The maximum amount of samples in history to keep.
 * @param valueReadType The sample-type type to read signal values as. Implicitly convert from actual type to
 * this one if conversion exists.
 * @param domainReadType The sample-type type to read signal domain as. Implicitly convert from actual type to
 * this one if conversion exists.
 */
inline TailReaderPtr TailReaderFromExisting(TailReaderPtr invalidatedReader, SizeT historySize, SampleType valueReadType, SampleType domainReadType)
{
    return TailReaderFromExisting_Create(invalidatedReader, historySize, valueReadType, domainReadType);
}

/*!
 * @brief Creates a new reader using the data of the existing one.
 * Used when a TailReader gets invalidated because of incompatible change in the signal descriptor.
 * This will reuse all of the read and connection info so no data is lost when changing to the new reader type.
 * @param invalidatedReader The reader from which to steal the read and connection info. If the passed-in
 * reader is not invalidated it will be invalidated after the call.
 * @param historySize The maximum amount of samples in history to keep.
 * @tparam TValueType The sample-type type to read signal values as. Implicitly convert from actual type to
 * this one if conversion exists.
 * @tparam TDomainType The sample-type type to read signal domain as. Implicitly convert from actual type to
 * this one if conversion exists.
 */
template <typename TValueType = double, typename TDomainType = ClockTick>
StreamReaderPtr TailReaderFromExisting(TailReaderPtr invalidatedReader, SizeT historySize)
{
    return TailReaderFromExisting(
        invalidatedReader,
        historySize,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType
    );
}

inline BlockReaderPtr BlockReader(SignalPtr signal,
                                  SizeT blockSize,
                                  SampleType valueReadType,
                                  SampleType domainReadType,
                                  ReadMode mode = ReadMode::Scaled)
{
    return BlockReader_Create(signal, blockSize, valueReadType, domainReadType, mode);
}

inline BlockReaderPtr BlockReaderFromPort(InputPortConfigPtr port,
                                  SizeT blockSize,
                                  SampleType valueReadType,
                                  SampleType domainReadType,
                                  ReadMode mode = ReadMode::Scaled)
{
    return BlockReaderFromPort_Create(port, blockSize, valueReadType, domainReadType, mode);
}

inline BlockReaderPtr BlockReaderFromExisting(const BlockReaderPtr& invalidatedReader,
                                              SizeT blockSize,
                                              SampleType valueReadType,
                                              SampleType domainReadType)
{
    return BlockReaderFromExisting_Create(invalidatedReader, valueReadType, domainReadType, blockSize);
}

template <typename TValueType = double, typename TDomainType = ClockTick>
BlockReaderPtr BlockReader(SignalPtr signal, SizeT blockSize, ReadMode mode = ReadMode::Scaled)
{
    return BlockReader(
        signal,
        blockSize,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType,
        mode
    );
}

template <typename TValueType = double, typename TDomainType = ClockTick>
BlockReaderPtr BlockReaderFromPort(InputPortConfigPtr port, SizeT blockSize, ReadMode mode = ReadMode::Scaled)
{
    return BlockReaderFromPort(
        port,
        blockSize,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType,
        mode
    );
}


template <typename TValueType = double, typename TDomainType = ClockTick>
BlockReaderPtr BlockReaderFromExisting(BlockReaderPtr invalidatedReader, SizeT blockSize)
{
    return BlockReaderFromExisting(
        invalidatedReader,
        blockSize,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType
    );
}

inline MultiReaderBuilderPtr MultiReaderBuilder()
{
    return MultiReaderBuilder_Create();
}

inline MultiReaderPtr MultiReaderFromBuilder(const MultiReaderBuilderPtr& builder)
{
    return MultiReaderFromBuilder_Create(builder);
}

inline MultiReaderPtr MultiReader(const ListPtr<ISignal>& signals,
                                  SampleType valueReadType,
                                  SampleType domainReadType,
                                  ReadMode mode = ReadMode::Scaled,  
                                  ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return MultiReader_Create(signals, valueReadType, domainReadType, mode, timeoutType);
}

inline MultiReaderPtr MultiReaderFromPort(ListPtr<IInputPortConfig> ports,
                                  SampleType valueReadType,
                                  SampleType domainReadType,
                                  ReadMode mode = ReadMode::Scaled,  
                                  ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return MultiReader_Create(ports, valueReadType, domainReadType, mode, timeoutType);
}

inline MultiReaderPtr MultiReaderEx(const ListPtr<ISignal>& signals,
                                    SampleType valueReadType,
                                    SampleType domainReadType,
                                    ReadMode mode = ReadMode::Scaled,
                                    ReadTimeoutType timeoutType = ReadTimeoutType::All,
                                    Int requiredCommonSampleRate = -1,
                                    bool startOnFullUnitOfDomain = false)
{
    return MultiReaderEx_Create(signals, valueReadType, domainReadType, mode, timeoutType, requiredCommonSampleRate, startOnFullUnitOfDomain);
}

inline MultiReaderPtr MultiReaderFromExisting(const MultiReaderPtr& invalidatedReader, SampleType valueReadType, SampleType domainReadType)
{
    return MultiReaderFromExisting_Create(invalidatedReader, valueReadType, domainReadType);
}

template <typename TValueType = double, typename TDomainType = ClockTick>
MultiReaderPtr MultiReader(ListPtr<ISignal> signals, ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return MultiReader(
        signals,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType,
        ReadMode::Scaled,
        timeoutType
    );
}
template <typename TValueType = double, typename TDomainType = ClockTick>
inline MultiReaderPtr MultiReaderFromPort(ListPtr<IInputPortConfig> ports,
                                  ReadMode mode = ReadMode::Scaled,  
                                  ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return MultiReader_Create(ports, SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType, mode, timeoutType);
}

template <typename TValueType = double, typename TDomainType = ClockTick>
MultiReaderPtr MultiReaderEx(ListPtr<ISignal> signals, ReadTimeoutType timeoutType = ReadTimeoutType::All, Int requiredCommonSampleRate = -1, bool startOnFullUnitOfDomain = false)
{
    return MultiReaderEx(signals,
                         SampleTypeFromType<TValueType>::SampleType,
                         SampleTypeFromType<TDomainType>::SampleType,
                         ReadMode::Scaled,
                         timeoutType,
                         requiredCommonSampleRate,
                         startOnFullUnitOfDomain);
}

template <typename TValueType = double, typename TDomainType = ClockTick>
MultiReaderPtr MultiReader(ListPtr<ISignal> signals, ReadMode mode, ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return MultiReader(
        signals,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType,
        mode,
        timeoutType
    );
}

template <typename TDomainType = ClockTick>
MultiReaderPtr MultiReaderRaw(ListPtr<ISignal> signals, ReadTimeoutType timeoutType = ReadTimeoutType::All)
{
    return MultiReader(
        signals,
        SampleType::Undefined,
        SampleTypeFromType<TDomainType>::SampleType,
        ReadMode::RawValue,
        timeoutType
    );
}

template <typename TValueType = double, typename TDomainType = ClockTick>
MultiReaderPtr MultiReaderFromExisting(MultiReaderPtr invalidatedReader)
{
    return MultiReaderFromExisting(
        invalidatedReader,
        SampleTypeFromType<TValueType>::SampleType,
        SampleTypeFromType<TDomainType>::SampleType
    );
}


END_NAMESPACE_OPENDAQ
