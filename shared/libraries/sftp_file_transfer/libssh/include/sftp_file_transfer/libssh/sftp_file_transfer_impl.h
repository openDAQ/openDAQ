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
#include <coretypes/string_ptr.h>
#include <opendaq/authentication_config_ptr.h>
#include <opendaq/logger_ptr.h>
#include <opendaq/logger_component_ptr.h>
#include <sftp_file_transfer/file_transfer.h>

#include <sftp_file_transfer/libssh/sftp_connection.h>

#include <mutex>

namespace daq::sftp::libssh
{

// IFileTransfer over SFTP, implemented with libssh. The connection is established in the constructor and closed on destruction.
class SftpFileTransferImpl final : public ImplementationOf<IFileTransfer>
{
public:
    SftpFileTransferImpl(const StringPtr& address, const AuthenticationConfigPtr& authenticationConfig, const LoggerPtr& logger);

    // IFileTransfer
    ErrCode INTERFACE_FUNC upload(IString* localPath, IString* remotePath) override;
    ErrCode INTERFACE_FUNC download(IString* remotePath, IString* localPath) override;

private:
    LoggerComponentPtr loggerComponent;
    std::mutex connectionSync;  // the connection is single-threaded (see SftpConnection)
    std::unique_ptr<SftpConnection> connection;
};

}
