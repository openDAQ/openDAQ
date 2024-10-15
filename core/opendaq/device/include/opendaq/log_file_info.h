/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/common.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

struct ILogFileInfoBuilder;

/*!
 * @ingroup opendaq_log_file_info
 * @addtogroup opendaq_log_file_info log_file_info
 * @{
 */

enum class LogFileEncodingType: uint32_t
{
    Utf8 = 0
};

DECLARE_OPENDAQ_INTERFACE(ILogFileInfo, IBaseObject)
{
    /*!
     * @brief Gets the id of the log file in format `getLocalPath() + "/" + getName()`.
     * @param[out] id The id of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    /*!
     * @brief Gets the local path of the log file. The local path can be not assigned as it is optional.
     * @param[out] localPath The local path of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getLocalPath(IString** localPath) = 0;

    /*!
     * @brief Gets the name of the log file.
     * @param[out] name The name of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets the description of the log file.
     * @param[out] description The description of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    /*!
     * @brief Gets the size of the log file in bytes.
     * @param[out] size The size of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getSize(SizeT* size) = 0;

    /*!
     * @brief Gets the encoding of the log file.
     * @param[out] encoding The encoding of the log file. Has a type of enum class `LogFileEncodingType`.
     */
    virtual ErrCode INTERFACE_FUNC getEncoding(LogFileEncodingType* encoding) = 0;

    /*!
     * @brief Gets the date of the last modification of the log file in ISO 8601 format.
     * @param[out] lastModified The date of the last modification of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getLastModified(IString** lastModified) = 0;
};

/*!@}*/
/*!
 * @ingroup opendaq_log_file_info
 * @addtogroup opendaq_log_file_info_factories Factories
 * @{
 */

/*!
 * @brief Creates an log file info with parameters.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, LogFileInfo, ILogFileInfo,
    IString*, localPath,
    IString*, name,
    IString*, description,
    LogFileEncodingType, encoding
)

/*!
 * @brief Creates an log file info from the builder.
 * @param builder The log file info builder.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, LogFileInfoFromBuilder, ILogFileInfo,
    ILogFileInfoBuilder*, builder
)

/*!@}*/

END_NAMESPACE_OPENDAQ
