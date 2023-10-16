/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/server_type.h>
#include <opendaq/device.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup structure_servers
 * @addtogroup structure_server Server
 * @{
 */

/*!
 * @brief Represents a server. The server provides access to the openDAQ device.
 * Depend of the implementation, it can support configuring the device, reading configuration, and data streaming.
 *
 * We do not make the server directly. But through the instance and module manager. For that reason, the server must be uniquely
 * defined with "serverType". The server is than created with the current root device, context and configuration object.
 *
 * Configuration of the server can be done with custom property object.
 * The configuration object is created with the corresponding ServerType object (IServerType::createDefaultConfig method).
 * For example, with a configuration object, we can define connection timeout.
 */
DECLARE_OPENDAQ_INTERFACE(IServer, IBaseObject)
{
    /*!
    * @brief Stops the server. This is called when we remove the server from the Instance or Instance is closing.
    */
    virtual ErrCode INTERFACE_FUNC stop() = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
