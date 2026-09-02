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

#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <coretypes/filesystem.h>
#include <opendaq/opendaq.h>

#include <file_recorder_module/common.h>
#include <file_recorder_module/file_format.h>

BEGIN_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE

/*!
 * @brief Records the packets of a single signal into one or more `.daqrec` files.
 *
 * The writer is a producer/consumer queue. The caller is the producer, enqueuing packets with
 * post() and then returning immediately to the acquisition thread. A background thread is the
 * consumer, dequeuing packets and writing them to the filesystem, so that file I/O never blocks
 * acquisition.
 *
 * A new file is started when the current one reaches the configured size limit, and also when the
 * signal's descriptor changes, so that the sample type and domain rule are constant within a
 * single file. Each file repeats the complete header and can therefore be replayed on its own.
 *
 * If an I/O or data processing error occurs, the current file is closed, a warning is logged, and
 * all subsequent packets are ignored. This mirrors the behavior of the basic CSV recorder: one bad
 * signal does not disturb the recording of the others.
 *
 * A writer can be given a sample limit, which it records up to exactly - truncating the packet
 * which crosses it - before closing its file and reporting itself finished.
 */
class SignalFileWriter final
{
public:
    /*!
     * @brief Creates a writer and starts its background thread. No file is opened until the first
     *     data packet arrives, so connecting a signal that never produces data leaves no files
     *     behind.
     *
     * @param directory The directory in which to create files. It is created if it does not exist.
     * @param signal The signal being recorded. Its name is used to build filenames, and its global
     *     ID is stored in the file header.
     * @param maxFileSizeBytes The size at which to roll over to a new file, or 0 for no limit. The
     *     limit is checked after each block, so a file may exceed it by up to one block.
     * @param sampleLimit The number of samples to record before finishing, or 0 to record until
     *     stopped. The packet crossing the limit is truncated, so exactly this many samples are
     *     stored.
     * @param onFinished Called once, from the background thread, when the sample limit is reached
     *     or the writer gives up after an error. May be empty.
     * @param loggerComponent The openDAQ logger object to use.
     */
    SignalFileWriter(const fs::path& directory,
                     const SignalPtr& signal,
                     std::uint64_t maxFileSizeBytes,
                     std::uint64_t sampleLimit,
                     std::function<void()> onFinished,
                     const LoggerComponentPtr& loggerComponent);

    /*!
     * @brief Stops the background thread and closes the current file. Packets still queued are
     *     written before the thread exits, so that stopping a recording does not truncate it.
     */
    ~SignalFileWriter();

    SignalFileWriter(const SignalFileWriter&) = delete;
    SignalFileWriter& operator=(const SignalFileWriter&) = delete;

    /*!
     * @brief Enqueues a packet to be recorded. A reference to the packet is held until the
     *     background thread has written it or this object is destroyed.
     * @param packet The event or data packet to record.
     */
    void post(const PacketPtr& packet);

    /*!
     * @brief Returns the path of the file currently being written, or an empty path if no file is
     *     open. Intended for logging and tests.
     */
    fs::path getCurrentFilename() const;

    /*!
     * @brief Returns true once this writer has stopped accumulating samples, either because its
     *     sample limit was reached or because it gave up after an error.
     */
    bool isFinished() const;

private:
    /*!
     * @brief How one file stores its samples. Fixed for the lifetime of a file: a packet whose
     *     layout differs from the open file's rolls over to a new one.
     */
    struct Layout
    {
        file_format::Header header;

        /*!
         * @brief How the domain of every block in the file is stored.
         */
        file_format::DomainKind domainKind = file_format::DomainKind::None;

        /*!
         * @brief True if value samples must be taken from the packet's calculated data rather
         *     than its raw buffer, because the signal's value rule is implicit.
         */
        bool materializeValues = false;

        /*!
         * @brief True if domain samples must be taken from the domain packet's calculated data,
         *     because its rule is implicit but not linear and so cannot be stored as an offset.
         */
        bool materializeDomain = false;

        /*!
         * @brief The number of bytes one stored value sample occupies.
         */
        std::size_t valueSampleSize = 0;

        /*!
         * @brief The number of bytes one stored domain sample occupies, or 0 if there is none.
         */
        std::size_t domainSampleSize = 0;

        bool sameAs(const Layout& other) const;
    };

    void threadMain();

    void processPacket(const PacketPtr& packet);
    void processEventPacket(const EventPacketPtr& packet);
    void processDataPacket(const DataPacketPtr& packet);

    void openNextFile(const Layout& layout);
    void closeFile();

    /*!
     * @brief Derives from @p packet how its samples must be stored, including the descriptor
     *     rewriting needed when rule-generated values have to be materialized.
     */
    Layout buildLayout(const DataPacketPtr& packet) const;

    void fail(const std::string& message);

    /*!
     * @brief Marks the writer as no longer accumulating samples, drops whatever is still queued
     *     and reports the writer finished. Calling it a second time does nothing.
     */
    void finish();

    const fs::path directory;
    const StringPtr signalName;
    const StringPtr signalGlobalId;
    const std::uint64_t maxFileSizeBytes;

    /*!
     * @brief The number of samples to record before finishing, or 0 for no limit.
     */
    const std::uint64_t sampleLimit;

    /*!
     * @brief Invoked, outside #mutex, when the writer finishes on its own.
     */
    const std::function<void()> onFinished;

    const std::string filenameStem;

    LoggerComponentPtr loggerComponent;

    mutable std::mutex mutex;
    std::condition_variable cv;
    std::queue<PacketPtr> queue;
    bool stopRequested = false;

    /*!
     * @brief Set once the writer has stopped accumulating samples. Read by isFinished() and by
     *     post(), which drops packets arriving afterwards.
     */
    bool finished = false;

    // The members below are owned by the background thread and are not protected by the mutex,
    // except for currentFilename which getCurrentFilename() reads.
    std::ofstream file;
    fs::path currentFilename;
    Layout currentLayout;
    std::uint64_t bytesWritten = 0;

    /*!
     * @brief The number of samples recorded so far, counted across all of this signal's parts and
     *     compared against #sampleLimit.
     */
    std::uint64_t samplesWritten = 0;

    Int partIndex = 0;
    bool failed = false;

    std::thread thread;
};

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
