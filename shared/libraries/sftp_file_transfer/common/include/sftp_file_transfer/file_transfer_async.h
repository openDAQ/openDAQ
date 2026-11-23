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
#include <coretypes/event.h>
#include <opendaq/component_status_container.h>
#include <sftp_file_transfer/file_transfer_operation.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IComponentStatusContainer, "opendaq")]
 * [interfaceLibrary(ICoreEventArgs, "coreobjects")]
 * [includeHeader("<coretypes/event_wrapper.h>")]
 */

/*!
 * @brief A connection to a remote file server over which files are transferred asynchronously: several transfers
 * run at the same time, driven by an I/O thread of the object, while the caller's thread is free.
 *
 * The object is connected and authenticated when created. If the connection is lost, it is restored automatically
 * (as configured), and the interrupted transfers continue where they stopped. Releasing the last reference closes the
 * connection and cancels the transfers that have not finished; the operation objects stay valid.
 *
 * The status container holds the status `ConnectionStatus` (see `ConnectionStatusName`), of the openDAQ enumeration
 * type `ConnectionStatusType`:
 * - `Connected`: the connection is up.
 * - `Reconnecting`: the connection was lost and is being restored; the status message tells why it was lost.
 * - `Unrecoverable`: the connection was lost and could not be restored; every unfinished transfer has failed and new
 *   ones fail when started. The status message tells why.
 *
 * Remote paths are interpreted by the server; relative ones are usually resolved against the home directory of the
 * logged-in user.
 */
DECLARE_OPENDAQ_INTERFACE(IFileTransferAsync, IBaseObject)
{
    /*!
     * @brief Gets the status container holding the status `ConnectionStatus`.
     * @param[out] statusContainer The status container.
     */
    virtual ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** statusContainer) = 0;

    // [templateType(event, IBaseObject, ICoreEventArgs)]
    /*!
     * @brief Gets the event triggered on every status change of the connection and of each of its transfers.
     * @param[out] event The event. It triggers with the object whose status changed, i.e. this object for
     * `ConnectionStatus` or an `IFileTransferOperation` for `TransferStatus`, and a CoreEventArgs object with the ID
     * `StatusChanged`, whose parameters hold the new value under the status name and the message under `Message`.
     * Handlers run on the I/O thread (see `IFileTransferOperation`). For a transfer, the operation's own event
     * triggers first.
     */
    virtual ErrCode INTERFACE_FUNC getOnStatusChanged(IEvent** event) = 0;

    /*!
     * @brief Creates an upload, copying a local file to the server, creating the remote file or overwriting it if it
     * exists. The transfer begins once `start` is called on the returned operation.
     * @param localPath Path of the local file to read.
     * @param remotePath Path of the file on the server to write.
     * @param[out] operation The upload, in the status `Created`.
     */
    virtual ErrCode INTERFACE_FUNC upload(IString* localPath, IString* remotePath, IFileTransferOperation** operation) = 0;

    /*!
     * @brief Creates a download, copying a file from the server to a local file, creating the local file or
     * overwriting it if it exists. The transfer begins once `start` is called on the returned operation.
     * @param remotePath Path of the file on the server to read.
     * @param localPath Path of the local file to write. Left incomplete if the transfer fails or is canceled midway.
     * @param[out] operation The download, in the status `Created`.
     */
    virtual ErrCode INTERFACE_FUNC download(IString* remotePath, IString* localPath, IFileTransferOperation** operation) = 0;
};

END_NAMESPACE_OPENDAQ
