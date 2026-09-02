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
 * The queue is bounded, because a filesystem which cannot keep up would otherwise have it grow
 * without limit, holding on to the acquisition's packets as it goes. When it fills up the writer
 * stops accepting packets and reports a failure, but still writes everything already queued, so
 * that the recording ends at a clean boundary instead of losing what it had already accepted.
 * Filling past a high water mark is reported before that happens, and the report is withdrawn
 * once the queue has drained well below it again.
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
     * @param signal The signal being recorded. Its global ID is stored in the file header.
     * @param filenameStem What every one of this signal's parts is named after, from
     *     buildFilenameStem() and made unique among the recording's signals by the recorder.
     * @param maxFileSizeBytes The size at which to roll over to a new file, or 0 for no limit. The
     *     limit is checked after each block, so a file may exceed it by up to one block.
     * @param sampleLimit The number of samples to record before finishing, or 0 to record until
     *     stopped. The packet crossing the limit is truncated, so exactly this many samples are
     *     stored.
     * @param maxQueuedBytes How much data may wait to be written before the writer gives up, or 0
     *     for no limit. A packet arriving at an empty queue is always accepted, so that a limit
     *     smaller than one packet still makes progress.
     * @param onStateChanged Called from the writer's own thread, and from the thread posting to
     *     it, whenever getStatus() would report something new. May be empty.
     * @param loggerComponent The openDAQ logger object to use.
     */
    SignalFileWriter(const fs::path& directory,
                     const SignalPtr& signal,
                     std::string filenameStem,
                     std::uint64_t maxFileSizeBytes,
                     std::uint64_t sampleLimit,
                     std::uint64_t maxQueuedBytes,
                     std::function<void()> onStateChanged,
                     const LoggerComponentPtr& loggerComponent);

    /*!
     * @brief Closes the writer, if close() has not already done it.
     */
    ~SignalFileWriter();

    /*!
     * @brief Stops the background thread and closes the current file. Packets still queued are
     *     written before the thread exits, so that stopping a recording does not truncate it.
     *
     * getStatus() only settles once this returns, because a writer which has run out of room
     * finishes writing what it accepted before it reports itself failed. Calling it more than
     * once does nothing.
     */
    void close();

    SignalFileWriter(const SignalFileWriter&) = delete;
    SignalFileWriter& operator=(const SignalFileWriter&) = delete;

    /*!
     * @brief Builds what one signal's parts are named after: its name, reduced to characters a
     *     filename can hold, and the time the recording started.
     *
     * Neither is unique on its own - two signals can share a name, and a recording restarted
     * within the same second repeats the timestamp - so the recorder makes the stem unique before
     * handing it over.
     */
    static std::string buildFilenameStem(const StringPtr& signalName);

    /*!
     * @brief The path of one part of a recording, and the only place a recording's filenames are
     *     formed. The player finds the following part by incrementing the number this appends.
     */
    static fs::path partPath(const fs::path& directory, const std::string& filenameStem, Int partIndex);

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
     * @brief What a writer has to say about itself, taken in one piece so that a reader of it
     *     never sees a half-updated writer.
     */
    struct Status
    {
        /*!
         * @brief True once the writer has stopped accumulating samples, because its sample limit
         *     was reached, because its queue filled up, or because it gave up after an error.
         */
        bool finished = false;

        /*!
         * @brief True if what stopped it was a failure rather than the sample limit.
         */
        bool failed = false;

        /*!
         * @brief True while the queue is above its high water mark, and until it has drained
         *     below the low one.
         */
        bool queueHigh = false;

        /*!
         * @brief What went wrong, when #failed is set.
         */
        std::string message;
    };

    /*!
     * @brief Returns this writer's state.
     */
    Status getStatus() const;

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
     * @brief Marks the writer as no longer accumulating samples and reports the new state.
     *     Calling it a second time does nothing.
     *
     * @param message What went wrong, or empty if the writer simply recorded everything it was
     *     asked for.
     * @param dropQueued Whether to let go of the packets still queued. They cannot be written
     *     after an I/O error, but a queue which merely filled up is still written out in full.
     */
    void finish(const std::string& message, bool dropQueued);

    /*!
     * @brief Returns how many bytes of memory @p packet occupies while it waits in the queue.
     *
     * A packet holds a reference to the acquisition's own buffer, so this is what the queue costs
     * the process, plus a fixed allowance per packet which also bounds a queue of packets that
     * carry no buffer of their own.
     */
    static std::uint64_t queuedBytesOf(const PacketPtr& packet);

    /*!
     * @brief Reports the writer's state to its owner, and must be called with #mutex released.
     */
    void notifyStateChanged() const;

    const fs::path directory;
    const StringPtr signalName;
    const StringPtr signalGlobalId;
    const std::uint64_t maxFileSizeBytes;

    /*!
     * @brief The number of samples to record before finishing, or 0 for no limit.
     */
    const std::uint64_t sampleLimit;

    /*!
     * @brief How much queued data is allowed before the writer gives up, or 0 for no limit.
     */
    const std::uint64_t maxQueuedBytes;

    /*!
     * @brief The amount of queued data at which the queue is reported as filling up, and the
     *     amount it has to drain to before that report is withdrawn.
     */
    const std::uint64_t highWatermark;
    const std::uint64_t lowWatermark;

    /*!
     * @brief Invoked, outside #mutex, whenever getStatus() would report something new.
     */
    const std::function<void()> onStateChanged;

    const std::string filenameStem;

    LoggerComponentPtr loggerComponent;

    /*!
     * @brief A packet waiting to be written, with what it costs to hold on to it.
     */
    struct QueuedPacket
    {
        PacketPtr packet;
        std::uint64_t bytes = 0;
    };

    mutable std::mutex mutex;
    std::condition_variable cv;
    std::queue<QueuedPacket> queue;
    bool stopRequested = false;

    /*!
     * @brief What the packets in #queue add up to, kept against #maxQueuedBytes.
     */
    std::uint64_t queuedBytes = 0;

    /*!
     * @brief True once #queue passed #highWatermark, until it falls back to #lowWatermark. The
     *     two differ so that a queue hovering around the mark does not report itself repeatedly.
     */
    bool queueHigh = false;

    /*!
     * @brief Set when a packet had to be turned away because the queue was full. What is already
     *     queued is still written before the writer closes its file.
     */
    bool queueOverflowed = false;

    /*!
     * @brief What stopped this writer, when what stopped it was a failure.
     */
    std::string failureMessage;

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
