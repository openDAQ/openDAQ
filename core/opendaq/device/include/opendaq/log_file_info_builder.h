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
     * @brief Gets the id of the log file. If the local path is not assigned, the id is equal to the `localPath + "/" + name`.
     * @param id The id of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    // [returnSelf()]
    /*!
     * @brief Sets the id of the log file. Oth
     * @param id The id of the log file.
     */
    virtual ErrCode INTERFACE_FUNC setId(IString* id) = 0;

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
     * @brief Gets the size of the log file in bytes.
     * @param[out] size The size of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getSize(SizeT* size) = 0;

    // [returnSelf()]
    /*!
     * @brief Sets the size of the log file in bytes.
     * @param size The size of the log file.
     */
    virtual ErrCode INTERFACE_FUNC setSize(SizeT size) = 0;

    /*!
     * @brief Gets the encoding of the log file.
     * @param[out] encoding The encoding of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getEncoding(IString** encoding) = 0;

    // [returnSelf()]
    /*!
     * @brief Sets the encoding of the log file.
     * @param encoding The encoding of the log file.
     */
    virtual ErrCode INTERFACE_FUNC setEncoding(IString* encoding) = 0;

    /*!
     * @brief Gets the date of the last modification of the log file in ISO 8601 format.
     * @param[out] lastModified The date of the last modification of the log file.
     */
    virtual ErrCode INTERFACE_FUNC getLastModified(IString** lastModified) = 0;

    // [returnSelf()]
    /*!
     * @brief Sets the date of the last modification of the log file in ISO 8601 format.
     * @param lastModified The date of the last modification of the log file.
     */
    virtual ErrCode INTERFACE_FUNC setLastModified(IString* lastModified) = 0;
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
