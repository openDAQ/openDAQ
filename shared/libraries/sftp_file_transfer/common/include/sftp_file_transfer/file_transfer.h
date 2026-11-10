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
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief A connection to a remote file server over which files can be transferred.
 *
 * The object is connected and authenticated for its whole lifetime. Releasing the last reference closes the
 * connection.
 *
 * Transfers are blocking: each call returns once the whole file has been transferred or has failed. Each object
 * performs one transfer at a time: a call made while another is in progress, e.g. from another thread, waits for it to
 * finish. Separate `IFileTransfer` objects transfer in parallel.
 *
 * Remote paths are interpreted by the server; relative ones are usually resolved against the home directory of the
 * logged-in user.
 */
DECLARE_OPENDAQ_INTERFACE(IFileTransfer, IBaseObject)
{
    /*!
     * @brief Copies a local file to the server, creating the remote file or overwriting it if it exists.
     * @param localPath Path of the local file to read.
     * @param remotePath Path of the file on the server to write.
     * @retval OPENDAQ_ERR_NOTFOUND if the local file cannot be opened, or the remote directory does not exist.
     * @retval OPENDAQ_ERR_ACCESSDENIED if the server does not allow writing the remote file.
     * @retval OPENDAQ_ERR_GENERALERROR on any other failure, including a lost connection.
     */
    virtual ErrCode INTERFACE_FUNC upload(IString* localPath, IString* remotePath) = 0;

    /*!
     * @brief Copies a file from the server to a local file, creating the local file or overwriting it if it exists.
     * @param remotePath Path of the file on the server to read.
     * @param localPath Path of the local file to write. Left incomplete if the transfer fails midway.
     * @retval OPENDAQ_ERR_NOTFOUND if the remote file does not exist.
     * @retval OPENDAQ_ERR_ACCESSDENIED if the server does not allow reading the remote file.
     * @retval OPENDAQ_ERR_GENERALERROR on any other failure, including a lost connection or an unwritable local file.
     */
    virtual ErrCode INTERFACE_FUNC download(IString* remotePath, IString* localPath) = 0;
};

END_NAMESPACE_OPENDAQ
