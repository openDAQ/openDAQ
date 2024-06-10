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
#include <opendaq/sample_reader.h>
#include <opendaq/signal.h>
#include <opendaq/input_port_config.h>
#include <opendaq/reader_status.h>

BEGIN_NAMESPACE_OPENDAQ

struct IBlockReaderBuilder;

/*#
 * [include(ISampleType)]
 * [interfaceSmartPtr(ISampleReader, GenericSampleReaderPtr)]
 */

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_block_reader Block reader
 * @{
 */

/*!
 * @brief A signal data reader that abstracts away reading of signal packets by keeping an
 * internal read-position and automatically advances it on subsequent reads. The difference to
 * a StreamReader is that instead of reading on per sample basis it always returns only a full block of samples.
 * This means that even if more samples are available they will not be read until there is enough of them to fill
 * at least one block.
 * @remark Currently only supports single-dimensional scalar sample-types and RangeInt64
 */
DECLARE_OPENDAQ_INTERFACE(IBlockReader, ISampleReader)
{
    // [arrayArg(blocks, count), arrayArg(count, 1)]
    /*!
     * @brief Copies at maximum the next `count` blocks of unread samples to the values buffer.
     * The amount actually read is returned through the `count` parameter.
     * @param[in] blocks The buffer that the samples will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` * `blockSize` amount of samples.
     * @param[in,out] count The maximum amount of blocks to be read. If the `count` is less than
     * available the parameter value is set to the actual amount and only the available
     * blocks are returned. The rest of the buffer is not modified or cleared.
     * @param timeoutMs The maximum amount of time in milliseconds to wait for the requested amount of blocks before returning.
     * @param[out] status: Represents the status of the reader.
     * - If the reader is invalid, IReaderStatus::getValid returns false.
     * - If an event packet was encountered during processing, IReaderStatus::isEventEncountered returns true.
     * - If the reading process is successful, ReaderStatus::isOk returns true, indicating that IReaderStatus::getValid is true and IReaderStatus::isEventEncountered is false.
     */
    virtual ErrCode INTERFACE_FUNC read(void* blocks, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr) = 0;

    // [arrayArg(dataBlocks, count), arrayArg(domainBlocks, count), arrayArg(count, 1)]
    /*!
     * @brief Copies at maximum the next `count` blocks of unread samples and clock-stamps to the `dataBlocks` and `domainBlocks` buffers.
     * The amount actually read is returned through the `count` parameter.
     * @param[in] dataBlocks The buffer that the samples will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` * `blockSize` amount of samples.
     * @param[in] domainBlocks The buffer that the domain values will be copied to.
     * The buffer must be a contiguous memory big enough to receive `count` * `blockSize` amount of clock-stamps.
     * @param[in,out] count The maximum amount of blocks to be read. If the `count` is less than
     * available the parameter value is set to the actual amount and only the available
     * blocks are returned. The rest of the buffer is not modified or cleared.
     * @param timeoutMs The maximum amount of time in milliseconds to wait for the requested amount of blocks before returning.
     * @param[out] status: Represents the status of the reader.
     * - If the reader is invalid, IReaderStatus::getValid returns false.
     * - If an event packet was encountered during processing, IReaderStatus::isEventEncountered returns true.
     * - If the reading process is successful, ReaderStatus::isOk returns true, indicating that IReaderStatus::getValid is true and IReaderStatus::isEventEncountered is false.
     */
    virtual ErrCode INTERFACE_FUNC readWithDomain(void* dataBlocks, void* domainBlocks, SizeT* count, SizeT timeoutMs = 0, IReaderStatus** status = nullptr) = 0;

    /*!
     * @brief The amount of samples the reader considers as one block.
     * @param[out] size The number of samples in a block.
     */
    virtual ErrCode INTERFACE_FUNC getBlockSize(SizeT* size) = 0;

    /*!
     * @brief The amount of block overlapping.
     * @param[out] overlap The overlap size in percents.
     */
    virtual ErrCode INTERFACE_FUNC getOverlap(SizeT* overlap) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, BlockReader,
    ISignal*, signal,
    SizeT, blockSize,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, BlockReaderFromExisting, IBlockReader,
    IBlockReader*, invalidatedReader,
    SampleType, valueReadType,
    SampleType, domainReadType,
    SizeT, blockSize
)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, BlockReaderFromPort, IBlockReader,
    IInputPortConfig*, port,
    SizeT, blockSize,
    SampleType, valueReadType,
    SampleType, domainReadType,
    ReadMode, mode
)

/*!
 * @brief Creates a BlockReader with Builder
 * @param builder BlockReader Builder
 */
//[factory(Hide)]
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, BlockReaderFromBuilder, IBlockReader,
    IBlockReaderBuilder*, builder
)

END_NAMESPACE_OPENDAQ
