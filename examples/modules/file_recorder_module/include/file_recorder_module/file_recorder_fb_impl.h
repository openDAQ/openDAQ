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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <coretypes/filesystem.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <file_recorder_module/common.h>
#include <file_recorder_module/signal_file_writer.h>

BEGIN_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE

/*!
 * @brief A recorder function block which writes the signals connected to its input ports into
 *     binary `.daqrec` files, one file per signal.
 *
 * The function block has a dynamic number of input ports: exactly one unconnected port is always
 * available, and connecting to it creates the next one. Signals can be connected and disconnected
 * while recording; doing so does not disturb the recording of the other signals.
 *
 * Recording is started through the `IRecorder` interface and stopped either through it or, in the
 * modes selected by RecordingMode, on its own once the requested number of samples or the
 * requested duration has been recorded. A recording which ends on its own goes through exactly
 * the same stop as an explicit one, differing only in what is logged and reported as the reason,
 * so that the properties unlock and a new recording can be started without stopping first. Each
 * recorded signal gets its own background writer thread, so that file I/O never blocks the
 * acquisition thread.
 *
 * A signal's recording is split across several files when the size limit set by MaxFileSizeMB is
 * reached, and also whenever the signal's descriptor changes. Every file repeats the full header
 * and can be replayed on its own by FilePlayerFbImpl.
 *
 * The properties become read-only while recording, so that a recording in progress cannot be
 * redirected or resized underneath itself. They are settable again once it is stopped.
 */
class FileRecorderFbImpl final : public FunctionBlockImpl<IFunctionBlock, IRecorder>
{
public:
    /*!
     * @brief The type ID of this function block.
     */
    static constexpr const char* TYPE_ID = "FileRecorder";

    /*!
     * @brief Contains constants for the names of tags assigned to this function block.
     */
    struct Tags
    {
        /*!
         * @brief A tag identifying this function block as a recorder.
         */
        static constexpr const char* RECORDER = "Recorder";
    };

    /*!
     * @brief What ends a recording, selected by the RecordingMode property.
     */
    enum class RecordingMode
    {
        /*!
         * @brief Only an explicit stop ends the recording.
         */
        Manual = 0,

        /*!
         * @brief The recording also ends once every recorded signal has stored the number of
         *     samples given by SampleCount.
         */
        SampleCount = 1,

        /*!
         * @brief The recording also ends once DurationSeconds have elapsed since it started.
         */
        Duration = 2,
    };

    /*!
     * @brief Contains constants for the names of properties supported by this function block.
     */
    struct Props
    {
        /*!
         * @brief The path to the directory where recordings are written. It is created if it does
         *     not exist. Relative paths are interpreted against the process's working directory.
         *
         * Read-only while recording.
         */
        static constexpr const char* PATH = "Path";

        /*!
         * @brief The size in megabytes at which a file is closed and the recording continues in
         *     the next part, or 0 for no limit. The limit is checked after each packet, so a file
         *     may exceed it by up to one packet.
         *
         * Read-only while recording.
         */
        static constexpr const char* MAX_FILE_SIZE_MB = "MaxFileSizeMB";

        /*!
         * @brief The RecordingMode which decides what ends the recording, besides an explicit
         *     stop, which ends it in every mode.
         *
         * Read-only while recording.
         */
        static constexpr const char* RECORDING_MODE = "RecordingMode";

        /*!
         * @brief The number of samples to record per signal in the SampleCount mode.
         *
         * The limit is applied to each recorded signal separately, so that every signal is
         * recorded in full and no signal is cut short by a faster one; the recording ends once
         * they have all reached it. A signal which never delivers that many samples therefore
         * keeps the recording running until it is stopped explicitly.
         *
         * Read-only while recording.
         */
        static constexpr const char* SAMPLE_COUNT = "SampleCount";

        /*!
         * @brief How long to record for, in seconds, in the Duration mode.
         *
         * The time is measured from the moment recording starts, not from the domain of the
         * samples, so a signal which stops delivering data does not extend it.
         *
         * Read-only while recording.
         */
        static constexpr const char* DURATION_SECONDS = "DurationSeconds";
    };

    /*!
     * @brief Creates and returns a type object describing this function block.
     */
    static FunctionBlockTypePtr createType();

    /*!
     * @brief Creates a new function block.
     * @param context The openDAQ context object.
     * @param parent The component object which will contain this function block.
     * @param localId The local identifier of this function block.
     * @param config A property object containing configuration data for this function block.
     */
    FileRecorderFbImpl(const ContextPtr& context,
                       const ComponentPtr& parent,
                       const StringPtr& localId,
                       const PropertyObjectPtr& config);

    ~FileRecorderFbImpl() override;

    /*!
     * @brief Starts the recording.
     * @retval OPENDAQ_SUCCESS The recording was started.
     * @retval OPENDAQ_ERR_INVALIDSTATE The recording was already active.
     */
    ErrCode INTERFACE_FUNC startRecording() override;

    /*!
     * @brief Stops the recording, flushing and closing every open file. Available in every
     *     RecordingMode, including the ones which also stop on their own, and never needed
     *     after one of those has ended a recording.
     * @retval OPENDAQ_SUCCESS The recording was stopped.
     * @retval OPENDAQ_ERR_INVALIDSTATE The recording was not active.
     */
    ErrCode INTERFACE_FUNC stopRecording() override;

    /*!
     * @brief Retrieves the current recording state.
     * @param isRecording A pointer to a boolean which is populated with the recording state.
     * @retval OPENDAQ_SUCCESS The state was stored.
     * @retval OPENDAQ_ERR_ARGUMENT_NULL @p isRecording is `nullptr`.
     */
    ErrCode INTERFACE_FUNC getIsRecording(Bool* isRecording) override;

    /*!
     * @brief Adds a writer for the newly connected signal and makes sure an unconnected input
     *     port remains available.
     */
    void onConnected(const InputPortPtr& port) override;

    /*!
     * @brief Closes the disconnected signal's files and removes the now-unused port.
     */
    void onDisconnected(const InputPortPtr& port) override;

    /*!
     * @brief Drains the port's connection and hands the packets to the signal's writer.
     */
    void onPacketReceived(const InputPortPtr& port) override;

protected:
    /*!
     * @brief Stops recording when the function block is deactivated.
     */
    void activeChanged() override;

private:
    void initProperties();

    /*!
     * @brief Returns a condition which is true exactly while recording, for use as a property's
     *     read-only flag. The flag is re-evaluated on every read, so the properties unlock again
     *     when the recording stops.
     */
    EvalValuePtr lockedWhileRecording();

    /*!
     * @brief Removes input ports which no longer have a signal and appends a fresh unconnected
     *     one, so that exactly one port is always free to be connected.
     */
    void updateInputPorts();

    /*!
     * @brief Creates a writer for every connected port which does not have one yet. Called when
     *     recording starts and whenever a signal is connected while it is active. A new writer
     *     joins the current recording's auto-stop state, so #autoStop must already be set.
     */
    void createWriters();

    /*!
     * @brief Returns true if there is at least one writer and none of them is still accumulating
     *     samples, which in the SampleCount mode means the recording is over.
     */
    bool allWritersFinished() const;

    /*!
     * @brief The state one recording's auto-stop thread waits on.
     *
     * It is owned jointly by the thread, by the writers whose completion wakes it, and by the
     * function block, so that ending a recording can abandon it without waiting: a thread woken
     * after its recording is over finds itself cancelled and exits.
     */
    struct AutoStop
    {
        std::mutex mutex;
        std::condition_variable cv;

        /*!
         * @brief Set when the recording this state belongs to has ended by other means.
         */
        bool cancelled = false;

        /*!
         * @brief Set when a writer has finished, to have the thread re-check the others.
         */
        bool wake = false;
    };

    /*!
     * @brief Why a recording ended. Only what is logged and reported differs between them: the
     *     stop itself is the same in every case.
     */
    enum class StopReason
    {
        Explicit,
        SampleCountReached,
        DurationElapsed,
    };

    /*!
     * @brief The whole of stopping a recording, shared by the explicit stop and by the auto-stop
     *     thread, so that the two leave the function block in exactly the same state.
     *
     * @param reason What ended the recording, which is logged and reported as the component's
     *     status message.
     * @param from The auto-stop state of the recording the caller means to end, or an empty
     *     pointer to end whichever recording is active. It keeps a thread which outlived its own
     *     recording from ending a later one.
     * @returns False if there was no such recording to stop, in which case nothing was changed.
     */
    bool stopRecording(StopReason reason, const std::shared_ptr<AutoStop>& from);

    /*!
     * @brief Describes @p reason for the log and for the component status.
     */
    static const char* describe(StopReason reason);


    /*!
     * @brief Waits for the recording's own end: the deadline in the Duration mode, or every
     *     writer reaching its sample limit in the SampleCount mode.
     * @param state The state of the recording this thread was started for. A later recording is
     *     left alone, in case this thread outlives the one it belongs to.
     */
    void autoStopMain(std::shared_ptr<AutoStop> state, RecordingMode mode, std::chrono::steady_clock::time_point deadline);

    /*!
     * @brief Tells the auto-stop thread of the current recording, if there is one, that it has
     *     nothing left to wait for. The thread is left to be joined later, because stopping is
     *     also reached from callbacks which hold the configuration lock the thread may want.
     */
    void cancelAutoStop();

    /*!
     * @brief Cancels the auto-stop thread and hands its handle over to be joined. The join is
     *     left to the caller, which must hold no lock the thread could still be waiting for.
     */
    std::thread takeAutoStopThread();

    /*!
     * @brief Has the auto-stop thread re-check whether every writer has finished. Called when one
     *     of them reports itself done and when a signal is disconnected.
     */
    void wakeAutoStop();

    /*!
     * @brief Destroys all writers, flushing and closing their files.
     */
    void clearWriters();

    /*!
     * @brief Looks up the writer recording @p port, holding #writersMutex only for the lookup
     *     itself so that the data path never waits on file I/O or on a configuration change in
     *     progress.
     * @returns The writer, or an empty pointer if @p port is not being recorded.
     */
    std::shared_ptr<SignalFileWriter> findWriter(IInputPort* port);

    /*!
     * @brief Caches the properties into the members the writers are created from. Only ever
     *     called while stopped, because the properties are read-only while recording.
     */
    void readProperties();

    /*!
     * @brief Guards #writers.
     *
     * A dedicated mutex rather than the function block's own locks: getAcquisitionLock() and
     * getRecursiveConfigLock() are two views of the same non-recursive mutex, so taking the
     * second while holding the first — which the configuration path would do on its way to the
     * writers — deadlocks. This one is only ever taken on its own.
     */
    mutable std::mutex writersMutex;

    /*!
     * @brief The writers, keyed by the input port whose signal they record. Held by shared
     *     pointers so that the data path can keep one alive while it posts packets outside
     *     #writersMutex.
     */
    std::unordered_map<IInputPort*, std::shared_ptr<SignalFileWriter>> writers;

    std::size_t portCount = 0;

    /*!
     * @brief Read by the acquisition thread on every packet, written under the configuration
     *     lock when recording starts or stops.
     */
    std::atomic_bool recording{false};

    fs::path path;
    std::uint64_t maxFileSizeBytes = 0;
    RecordingMode recordingMode = RecordingMode::Manual;
    std::uint64_t sampleCount = 0;
    std::chrono::steady_clock::duration duration{};

    /*!
     * @brief The state of the current recording's auto-stop thread, or empty if the mode does not
     *     need one.
     *
     * Guarded by the configuration lock, like the recording state it belongs to. The AutoStop's
     * own mutex, which a writer's thread takes to report itself finished, is never held while
     * taking any other lock.
     */
    std::shared_ptr<AutoStop> autoStop;

    /*!
     * @brief The auto-stop thread of the current recording, or of the last one if it has not been
     *     joined yet. A stopped recording leaves its thread here rather than joining it, because
     *     the thread may be waiting for the very configuration lock the stop is holding; the next
     *     start, or this function block's destructor, joins it once that lock is free.
     */
    std::thread autoStopThread;
};

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
