/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <connection/connection.h>
#include <corestructure/controller.h>

BEGIN_NAMESPACE_DEWESOFT_RT_CONNECTION

/*#
* [includeHeader("<corestructure/controller_ptr.h>")]
* [interfaceNamespace(IDewesoftConnection, "Dewesoft::RT::Connection::")]
* [interfaceNamespace(IController, "Dewesoft::RT::Core::")]
*/
DECLARE_RT_INTERFACE(IDewesoftConnection, Core::IBaseObject)
{
    DEFINE_INTFID("IDewesoftConnection")

    virtual Core::ErrCode INTERFACE_FUNC getController(Core::IController** controller) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, DewesoftConnection, IDewesoftConnection, Core::IString*, modulePath)

END_NAMESPACE_DEWESOFT_RT_CONNECTION
