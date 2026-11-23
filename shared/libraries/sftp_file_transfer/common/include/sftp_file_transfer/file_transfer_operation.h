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

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceLibrary(IComponentStatusContainer, "opendaq")]
 * [interfaceLibrary(ICoreEventArgs, "coreobjects")]
 * [includeHeader("<coretypes/event_wrapper.h>")]
 */

/*!
 * @brief One upload or download of an `IFileTransferAsync` object.
 *
 * Created by `IFileTransferAsync::upload` or `download` in the status `Created`; nothing is transferred until
 * `start` is called, so handlers can be added to `getOnStatusChanged` first without missing a change.
 *
 * The status container holds the status `TransferStatus` (see `FileTransferStatusName`), of the enumeration type
 * `FileTransferStatusType`:
 * - `Created`: not started yet.
 * - `Queued`: started, waiting for one of the connection's transfer slots or for the connection to be restored.
 * - `Running`: transferring.
 * - `Paused`: paused by `pause`, until `resume`.
 * - `Completed`, `Failed`, `Canceled`: finished; the status does not change any more. On `Failed`, the status message
 *   describes the error.
 *
 * Status changes happen on the I/O thread of the file transfer object, except that `cancel` on a transfer that was
 * not started changes the status on the calling thread. Handlers of `getOnStatusChanged` run where the status
 * changes, so they should return quickly; they may call `start`, `pause`, `resume` or `cancel`, but not `wait`.
 *
 * `start`, `pause`, `resume` and `cancel` only request the change and return at once; the status changes once the I/O
 * thread has carried it out. A running transfer pauses or cancels after the SFTP request in progress has completed.
 */
DECLARE_OPENDAQ_INTERFACE(IFileTransferOperation, IBaseObject)
{
    /*!
     * @brief Gets the status container holding the status `TransferStatus`.
     * @param[out] statusContainer The status container.
     */
    virtual ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** statusContainer) = 0;

    // [templateType(event, IFileTransferOperation, ICoreEventArgs)]
    /*!
     * @brief Gets the event triggered on every change of the status `TransferStatus` or of its message.
     * @param[out] event The event. It triggers with the operation and a CoreEventArgs object with the ID
     * `StatusChanged`, whose parameters hold the new value under `TransferStatus` and the message under `Message`.
     */
    virtual ErrCode INTERFACE_FUNC getOnStatusChanged(IEvent** event) = 0;

    /*!
     * @brief Starts the transfer: `Created` becomes `Queued`, and `Running` once a transfer slot is free.
     * @retval OPENDAQ_ERR_INVALIDSTATE if the transfer was already started, or the file transfer object was released.
     */
    virtual ErrCode INTERFACE_FUNC start() = 0;

    /*!
     * @brief Pauses a queued or running transfer; ignored in other states. A transfer paused while running keeps its
     * transfer slot (its SFTP channel and open remote file), unless the connection is lost meanwhile.
     */
    virtual ErrCode INTERFACE_FUNC pause() = 0;

    /*!
     * @brief Resumes a paused transfer. Ignored in other states.
     */
    virtual ErrCode INTERFACE_FUNC resume() = 0;

    /*!
     * @brief Cancels the transfer unless it has finished already. A partly uploaded remote file or partly downloaded
     * local file is left as is. A transfer that was not started is canceled before this call returns.
     */
    virtual ErrCode INTERFACE_FUNC cancel() = 0;

    /*!
     * @brief Blocks until the transfer has finished and the status change handlers have run.
     * @retval OPENDAQ_SUCCESS if the transfer completed or was canceled.
     * @retval OPENDAQ_ERR_NOTFOUND, OPENDAQ_ERR_ACCESSDENIED or OPENDAQ_ERR_GENERALERROR if it failed, as the blocking
     * `IFileTransfer::upload` and `download` report the same error.
     * @retval OPENDAQ_ERR_INVALIDSTATE if the transfer was not started, or if called from the I/O thread (e.g. a
     * status change handler), where it would wait for itself.
     */
    virtual ErrCode INTERFACE_FUNC wait() = 0;

    /*!
     * @brief Gets the path of the local file.
     * @param[out] path The local path.
     */
    virtual ErrCode INTERFACE_FUNC getLocalPath(IString** path) = 0;

    /*!
     * @brief Gets the path of the remote file.
     * @param[out] path The remote path.
     */
    virtual ErrCode INTERFACE_FUNC getRemotePath(IString** path) = 0;

    /*!
     * @brief Gets the number of bytes transferred so far: bytes confirmed by the server for an upload, bytes written to
     * the local file for a download.
     * @param[out] bytes The number of bytes.
     */
    virtual ErrCode INTERFACE_FUNC getBytesTransferred(SizeT* bytes) = 0;

    /*!
     * @brief Gets the size of the file being transferred: the local file for an upload, the remote file for a
     * download. 0 until known, i.e. before the transfer runs, or if the server does not report the size.
     * @param[out] bytes The number of bytes.
     */
    virtual ErrCode INTERFACE_FUNC getTotalBytes(SizeT* bytes) = 0;
};

END_NAMESPACE_OPENDAQ
