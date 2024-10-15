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
#include <opendaq/log_file_info.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_log_file_info
 * @addtogroup opendaq_log_file_info log_file_info_builder
 * @{
 */


DECLARE_OPENDAQ_INTERFACE(ILogFileInfoBuilder, IBaseObject)
{
    /*!
     * @brief Builds the log file info.
     * @param[out] logFileInfo The log file info.
     */
    virtual ErrCode INTERFACE_FUNC build(ILogFileInfo** logFileInfo) = 0;

    /*!
     * @brief Gets the local path of the log file. The local path can be not assigned as it is optional.
     * @param[out] localPath The local path of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getLocalPath(IString** localPath) = 0;

    // [returnSelf()]
    /*!
     * @brief Sets the local path of the log file. The local path can be not assigned as it is optional.
     * @param localPath The local path of the log file.
     */
    virtual ErrCode INTERFACE_FUNC setLocalPath(IString* localPath) = 0;

    /*!
     * @brief Gets the name of the log file.
     * @param[out] name The name of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    // [returnSelf()]
    /*!
     * @brief Sets the name of the log file.
     * @param name The name of the log file.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;

    /*!
     * @brief Gets the description of the log file.
     * @param[out] description The description of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getDescription(IString** description) = 0;

    // [returnSelf()]
    /*!
     * @brief Sets the description of the log file.
     * @param description The description of the log file.
     */
    virtual ErrCode INTERFACE_FUNC setDescription(IString* description) = 0;

    /*!
     * @brief Gets the encoding of the log file.
     * @param[out] encoding The encoding of the log file. Has a type of enum class `LogFileEncodingType`.
     */
    virtual ErrCode INTERFACE_FUNC getEncoding(LogFileEncodingType* encoding) = 0;

    // [returnSelf()]
    /*!
     * @brief Sets the encoding of the log file.
     * @param encoding The encoding of the log file. Has a type of enum class `LogFileEncodingType`.
     */
    virtual ErrCode INTERFACE_FUNC setEncoding(LogFileEncodingType encoding) = 0;
};

/*!@}*/
/*!
 * @ingroup opendaq_log_file_info
 * @addtogroup opendaq_log_file_info_factories Factories
 * @{
 */

/*!
 * @brief Creates an LogFileInfo Builder with no parameters configured.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, LogFileInfoBuilder, ILogFileInfoBuilder)

/*!@}*/

END_NAMESPACE_OPENDAQ
