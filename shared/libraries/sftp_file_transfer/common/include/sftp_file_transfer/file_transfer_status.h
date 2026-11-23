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
#include <coretypes/enumeration_type_factory.h>
#include <coretypes/list_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Name of the status of a file transfer connection, in the status container of `IFileTransferAsync`. Its type
 * is `ConnectionStatusType`.
 */
inline constexpr const char* ConnectionStatusName = "ConnectionStatus";

/*!
 * @brief Name of the status of a transfer, in the status container of `IFileTransferOperation`. Its type is
 * `FileTransferStatusType`.
 */
inline constexpr const char* FileTransferStatusName = "TransferStatus";

/*!
 * @brief Creates the enumeration type of the status `TransferStatus`. Its values, in order: `Created`, `Queued`,
 * `Running`, `Paused`, `Completed`, `Failed`, `Canceled` (see `IFileTransferOperation`).
 */
inline EnumerationTypePtr FileTransferStatusType()
{
    return EnumerationType("FileTransferStatusType",
                           List<IString>("Created", "Queued", "Running", "Paused", "Completed", "Failed", "Canceled"));
}

/*!
 * @brief Creates the enumeration type of the status `ConnectionStatus`: the openDAQ `ConnectionStatusType`, with the
 * values `Connected`, `Reconnecting`, `Unrecoverable` and `Removed` (the last one is not used for file transfers).
 */
inline EnumerationTypePtr FileTransferConnectionStatusType()
{
    return EnumerationType("ConnectionStatusType", List<IString>("Connected", "Reconnecting", "Unrecoverable", "Removed"));
}

END_NAMESPACE_OPENDAQ
