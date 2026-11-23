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
#include <coreobjects/property_object_ptr.h>
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/component_status_container_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <sftp_file_transfer/file_transfer_async_ptr.h>

#include <sftp_file_transfer/libssh2/sftp_async_engine.h>

#include <memory>

namespace daq::sftp::libssh2
{

// IFileTransferAsync over SFTP, implemented with libssh2 in non-blocking mode (see SftpAsyncEngine). The connection is
// established in the constructor; releasing the object cancels unfinished transfers and closes it.
class SftpFileTransferAsyncImpl final : public ImplementationOf<IFileTransferAsync>
{
public:
    SftpFileTransferAsyncImpl(const StringPtr& address,
                              const AuthenticationConfigPtr& authenticationConfig,
                              const LoggerPtr& logger,
                              const PropertyObjectPtr& config);
    ~SftpFileTransferAsyncImpl() override;

    // IFileTransferAsync
    ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** statusContainer) override;
    ErrCode INTERFACE_FUNC getOnStatusChanged(IEvent** event) override;
    ErrCode INTERFACE_FUNC upload(IString* localPath, IString* remotePath, IFileTransferOperation** operation) override;
    ErrCode INTERFACE_FUNC download(IString* remotePath, IString* localPath, IFileTransferOperation** operation) override;

private:
    ErrCode createOperation(bool upload, IString* localPath, IString* remotePath, IFileTransferOperation** operation);
    void connectionStatusChanged(ConnectionStatus status, const StringPtr& message);
    void statusChanged(const CoreEventArgsPtr& args);

    LoggerComponentPtr loggerComponent;
    EnumerationTypePtr transferStatusType;
    EnumerationTypePtr connectionStatusType;
    EventEmitter<const BaseObjectPtr, const CoreEventArgsPtr> onStatusChanged;
    ComponentStatusContainerPtr statusContainer;
    std::shared_ptr<SftpAsyncEngine> engine;  // created last: its I/O thread uses the members above
};

}
