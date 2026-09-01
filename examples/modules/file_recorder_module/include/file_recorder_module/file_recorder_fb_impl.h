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
#include <cstdint>
#include <memory>
#include <mutex>
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
 * Recording is started and stopped through the `IRecorder` interface. Each recorded signal gets its
 * own background writer thread, so that file I/O never blocks the acquisition thread.
 *
 * A signal's recording is split across several files when the size limit set by MaxFileSizeMB is
 * reached, and also whenever the signal's descriptor changes. Every file repeats the full header
 * and can be replayed on its own by FilePlayerFbImpl.
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
     * @brief Contains constants for the names of properties supported by this function block.
     */
    struct Props
    {
        /*!
         * @brief The path to the directory where recordings are written. It is created if it does
         *     not exist. Relative paths are interpreted against the process's working directory.
         */
        static constexpr const char* PATH = "Path";

        /*!
         * @brief The size in megabytes at which a file is closed and the recording continues in
         *     the next part, or 0 for no limit. The limit is checked after each packet, so a file
         *     may exceed it by up to one packet.
         */
        static constexpr const char* MAX_FILE_SIZE_MB = "MaxFileSizeMB";
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
     * @brief Stops the recording, flushing and closing every open file.
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
     * @brief Removes input ports which no longer have a signal and appends a fresh unconnected
     *     one, so that exactly one port is always free to be connected.
     */
    void updateInputPorts();

    /*!
     * @brief Creates a writer for every connected port which does not have one yet. Called when
     *     recording starts and whenever a signal is connected while it is active.
     */
    void createWriters();

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
};

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
