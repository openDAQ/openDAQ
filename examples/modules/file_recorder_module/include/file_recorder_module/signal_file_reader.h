/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <cstddef>
#include <fstream>
#include <vector>

#include <coretypes/filesystem.h>
#include <opendaq/opendaq.h>

#include <file_recorder_module/common.h>
#include <file_recorder_module/file_format.h>

BEGIN_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE

/*!
 * @brief Reads a `.daqrec` file written by SignalFileWriter, a piece at a time.
 *
 * The file is opened once and read incrementally: only the header and the samples currently being
 * replayed are held in memory, so a recording may be far larger than available RAM.
 *
 * Samples are handed out as runs. A run never spans two blocks, so a pause in the recording is
 * always a run boundary, and the caller can honor the pause simply by waiting between runs.
 */
class SignalFileReader final
{
public:
    /*!
     * @brief A group of samples which are contiguous both in the file and in the domain.
     */
    struct Run
    {
        /*!
         * @brief How this run's domain is described, copied from the block it came from.
         */
        file_format::DomainKind domainKind = file_format::DomainKind::None;

        /*!
         * @brief The number of samples in the run.
         */
        std::size_t sampleCount = 0;

        /*!
         * @brief The run's value samples, in the raw layout of the header's value descriptor.
         */
        std::vector<char> valueData;

        /*!
         * @brief The run's domain. Empty when there is no domain; one raw sample holding the
         *     run's first tick when the domain is implicit; one raw sample per sample when it is
         *     explicit.
         */
        std::vector<char> domainData;
    };

    /*!
     * @brief Opens @p path and reads its header.
     * @throws std::runtime_error The file cannot be opened, is not a recording, or its header is
     *     malformed, truncated, or written for a different byte order.
     */
    explicit SignalFileReader(const fs::path& path);

    /*!
     * @brief Returns the decoded file header, including the recorded signal's descriptors.
     */
    const file_format::Header& getHeader() const;

    /*!
     * @brief Returns true if the recorded signal had a domain signal.
     */
    bool hasDomain() const;

    /*!
     * @brief Returns the number of bytes one raw value sample occupies.
     */
    std::size_t getValueSampleSize() const;

    /*!
     * @brief Returns the number of bytes one raw domain sample occupies, or 0 if there is none.
     */
    std::size_t getDomainSampleSize() const;

    /*!
     * @brief Returns the number of seconds one domain tick represents, or 0 if the recording's
     *     domain descriptor does not specify a tick resolution.
     */
    double getTickResolutionSeconds() const;

    /*!
     * @brief Reads the next run of at most @p maxSamples samples.
     * @param maxSamples The largest number of samples to return. Must be greater than 0.
     * @param run Populated with the samples read.
     * @returns False if the end of the file was reached and @p run was not populated.
     * @throws std::runtime_error The file is truncated or malformed.
     */
    bool readRun(std::size_t maxSamples, Run& run);

    /*!
     * @brief Seeks back to the first block, so that the recording can be replayed again.
     */
    void rewind();

    /*!
     * @brief Returns the domain tick of the sample at @p index within @p run, as a double.
     *
     * The result is meant for timing arithmetic. Ticks which are emitted into output packets are
     * taken from the run's raw bytes instead, so that their exact values survive.
     *
     * @throws std::runtime_error The run has no domain.
     */
    double tickAt(const Run& run, std::size_t index) const;

    /*!
     * @brief Returns the domain tick of the sample at @p index within @p run, exactly.
     *
     * Use this for ticks which are emitted into output packets, where rounding through a double
     * would corrupt a nanosecond-resolution integer domain.
     *
     * @throws std::runtime_error The run has no domain, or the domain sample type is floating
     *     point, for which tickAt() is the exact accessor.
     */
    Int tickAtAsInt(const Run& run, std::size_t index) const;

    /*!
     * @brief Returns the linear rule's delta in ticks, or 0 if the recorded domain is not linear.
     */
    Int getDomainDeltaInt() const;

    /*!
     * @brief Returns the linear rule's delta in ticks as a double, or 0 if it is not linear.
     */
    double getDomainDeltaDouble() const;

    /*!
     * @brief Returns true if the recorded domain's sample type is floating point.
     */
    bool isDomainFloatingPoint() const;

private:
    /*!
     * @brief Reads the header of the next block and positions the stream at its payload.
     * @returns False at end of file.
     */
    bool beginNextBlock();

    std::ifstream file;
    file_format::Header header;
    std::streamoff dataStart = 0;

    std::size_t valueSampleSize = 0;
    std::size_t domainSampleSize = 0;
    SampleType domainSampleType = SampleType::Undefined;
    bool domainIsFloatingPoint = false;

    /*!
     * @brief The linear rule's delta, used to advance the start tick when an implicit block is
     *     handed out as several runs. Held in both forms so that integer domains stay exact.
     */
    Int domainDeltaInt = 0;
    double domainDeltaDouble = 0.0;

    file_format::BlockHeader currentBlock;
    std::size_t consumedInBlock = 0;

    /*!
     * @brief The raw first tick of the current block, for implicit domains.
     */
    std::vector<char> blockDomainStart;

    /*!
     * @brief The offset of the current block's first value sample.
     */
    std::streamoff blockValueBase = 0;

    /*!
     * @brief The offset of the current block's first domain sample, for explicit domains. Also
     *     the offset one past the block's value samples, and so the end of a block which has no
     *     explicit domain.
     */
    std::streamoff blockDomainBase = 0;
};

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
