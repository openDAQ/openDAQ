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
#include <coretypes/impl.h>
#include <coretypes/event_emitter.h>
#include <coretypes/enumeration_type_ptr.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/core_event_args_ptr.h>
#include <opendaq/component_status_container_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <sftp_file_transfer/file_transfer_operation_ptr.h>

#include <sftp_file_transfer/libssh2/sftp_async_engine.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>

namespace daq::sftp::libssh2
{

// IFileTransferOperation of SftpFileTransferAsyncImpl. Requests go to the engine; the engine reports back through the
// methods below, on its I/O thread.
class SftpFileTransferOperationImpl final : public ImplementationOf<IFileTransferOperation>
{
public:
    // The engine is held weakly: an operation may outlive the file transfer object. connectionEvent is the file
    // transfer object's status change event, triggered after the operation's own.
    SftpFileTransferOperationImpl(const std::weak_ptr<SftpAsyncEngine>& engine,
                                  bool upload,
                                  const StringPtr& localPath,
                                  const StringPtr& remotePath,
                                  const EnumerationTypePtr& statusType,
                                  const EventPtr<const BaseObjectPtr, const CoreEventArgsPtr>& connectionEvent,
                                  const LoggerComponentPtr& loggerComponent);

    // IFileTransferOperation
    ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** statusContainer) override;
    ErrCode INTERFACE_FUNC getOnStatusChanged(IEvent** event) override;
    ErrCode INTERFACE_FUNC start() override;
    ErrCode INTERFACE_FUNC pause() override;
    ErrCode INTERFACE_FUNC resume() override;
    ErrCode INTERFACE_FUNC cancel() override;
    ErrCode INTERFACE_FUNC wait() override;
    ErrCode INTERFACE_FUNC getLocalPath(IString** path) override;
    ErrCode INTERFACE_FUNC getRemotePath(IString** path) override;
    ErrCode INTERFACE_FUNC getBytesTransferred(SizeT* bytes) override;
    ErrCode INTERFACE_FUNC getTotalBytes(SizeT* bytes) override;

    // Used by the engine.
    bool isUpload() const noexcept;
    StringPtr localPath() const;
    StringPtr remotePath() const;
    TransferStatus status() const;
    void setTotalBytes(std::uint64_t bytes) noexcept;
    void setBytesTransferred(std::uint64_t bytes) noexcept;

    // Sets the status, with the error of a failed transfer, and triggers the status change events. A finished status
    // (Completed, Failed, Canceled) is final: later changes are ignored, and wait returns once the events have run.
    void setStatus(TransferStatus status, ErrCode errorCode = OPENDAQ_SUCCESS, const StringPtr& message = "");

private:
    void statusChanged(const CoreEventArgsPtr& args);

    const std::weak_ptr<SftpAsyncEngine> engine;
    const bool upload;
    const StringPtr local;
    const StringPtr remote;
    const EnumerationTypePtr statusType;
    const EventPtr<const BaseObjectPtr, const CoreEventArgsPtr> connectionEvent;
    const LoggerComponentPtr loggerComponent;

    EventEmitter<const FileTransferOperationPtr, const CoreEventArgsPtr> onStatusChanged;
    ComponentStatusContainerPtr statusContainer;

    std::atomic<std::uint64_t> totalBytes{0};
    std::atomic<std::uint64_t> bytesTransferred{0};

    mutable std::mutex sync;
    std::condition_variable finishedChanged;
    bool started = false;
    bool finished = false;  // the final status is set and its events have run
    TransferStatus currentStatus = TransferStatus::Created;
    ErrCode errorCode = OPENDAQ_SUCCESS;
    StringPtr errorMessage;
};

}
