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

#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <parquet_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE

class ParquetWriter;

/*!
 * @brief A function block recording data from its input signals into a Parquet
 *     file.
 *
 * A separate Parquet file is created for each recorded signal, in a directory specified by the user
 * via a property. Recording can be started and stopped by calling member functions or by setting
 * a property. Signals can be dynamically connected and disconnected.
 *
 * Packet handling takes place in scheduled tasks.
 */
class ParquetRecorderImpl final : public FunctionBlockImpl<IFunctionBlock, IRecorder>
{
public:
    /*!
     * @brief Creates and returns a type object describing this function block.
     * @returns A populated function block type object.
     */
    static FunctionBlockTypePtr createType();

    /*!
     * @brief Creates a new function block.
     * @param context The openDAQ context object.
     * @param parent The component object which will contain this function block.
     * @param localId The local identifier of this function block.
     * @param config A property object containing configuration data for this function block.
     */
    ParquetRecorderImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);

    ~ParquetRecorderImpl();

    /*!
     * @brief Starts the recording, if it was not already started.
     * @return OPENDAQ_SUCCESS on successful start or OPENDAQ_ERR_INVALIDSTATE if the recording
     *     was already started.
     */
    ErrCode INTERFACE_FUNC startRecording() override;

    /*!
     * @brief Stops the recording, if it was started.
     * @return OPENDAQ_SUCCESS on successful stop or OPENDAQ_ERR_INVALIDSTATE if the recording
     *     was not active.
     */
    ErrCode INTERFACE_FUNC stopRecording() override;

    /*!
     * @brief Retrieves the current recording state.
     * @param isRecording A pointer to a boolean which is populated with the recording state.
     * @retval OPENDAQ_SUCCESS if the recording state was successfully stored.
     * @retval OPENDAQ_ERR_ARGUMENT_NULL if @p isRecording is `nullptr`.
     */
    ErrCode INTERFACE_FUNC getIsRecording(Bool* isRecording) override;

    /*!
     * @brief Called when a new signal is connected to the input port.
     * @param port The input port that was connected.
     */
    void onConnected(const InputPortPtr& port) override;

    /*!
     * @brief Called when a signal is disconnected from the input port.
     * @param port The input port that was disconnected.
     */
    void onDisconnected(const InputPortPtr& port) override;

    /*!
     * @brief Records a packet.
     *
     * @param port The input port on which the packet was received.
     */
    void onPacketReceived(const InputPortPtr& port) override;

protected:
    /*!
     * @brief Stops recording when the component is deactivated.
     */
    virtual void activeChanged() override;

private:
    void addProperties();
    void addInputPort();
    void reconfigure();
    void clearWriters();
    std::shared_ptr<ParquetWriter> findWriterForSignal(IInputPort* port);

    std::unordered_map<IInputPort*, std::shared_ptr<ParquetWriter>> writers;
    SchedulerPtr scheduler;
    std::atomic_uint32_t portCount = 0;
    std::atomic_bool recording = false;
};

END_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE
