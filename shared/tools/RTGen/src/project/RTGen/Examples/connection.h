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
#include <coretypes/coretypes.h>
#include "common.h"

BEGIN_NAMESPACE_DEWESOFT_RT_CONNECTION

/*#
* [templated]
* [includeHeader("corestructure/controller_ptr.h")]
* [interfaceNamespace(IConnection, "Dewesoft::RT::Connection::")]
*/
DECLARE_RT_INTERFACE_EX(IConnection, Core::IBaseObject)
{
    DEFINE_CUSTOM_INTFID("IEventArgs", "Connection.RT.Dewesoft")

    virtual Core::ErrCode INTERFACE_FUNC startConnection() = 0;
    virtual Core::ErrCode INTERFACE_FUNC stopConnection() = 0;

    //controller
    virtual Core::ErrCode INTERFACE_FUNC configure() = 0;
    virtual Core::ErrCode INTERFACE_FUNC initialize() = 0;
    virtual Core::ErrCode INTERFACE_FUNC start() = 0;
    virtual Core::ErrCode INTERFACE_FUNC stop() = 0;
    virtual Core::ErrCode INTERFACE_FUNC finalize() = 0;

    virtual Core::ErrCode INTERFACE_FUNC getData() = 0;
};

END_NAMESPACE_DEWESOFT_RT_CONNECTION
