/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Enumeration of available client types
 */
enum class ClientType : EnumType
{
    Control = 0, /// A client connected to the server can view and configure the device just like other Control clients
    ExclusiveControl = 1, /// A client connected to the server can view the device and it is the only one who can configure it
    ViewOnly = 2 /// A client connected to the server can view the device but can not modify it
};

/*!
 * @brief A class containing helper methods for working with ClientType enum
 */
class ClientTypeTools
{
public:
    static void DefineConfigProperties(const PropertyObjectPtr& obj)
    {
        auto clientTypes = Dict<IInteger, IString>();
        clientTypes.set(static_cast<Int>(ClientType::Control), "Control");
        clientTypes.set(static_cast<Int>(ClientType::ExclusiveControl), "Exclusive Control");
        clientTypes.set(static_cast<Int>(ClientType::ViewOnly), "View Only");

        const auto clientTypeProp =
            SparseSelectionPropertyBuilder("ClientType", clientTypes, (Int) ClientType::Control)
                .setDescription("Specifies the client's connection type. Control and Exclusive Control clients can modify the device, while "
                                "View Only clients can only read from the device. When an Exclusive Control client is connected, no other "
                                "Control or Exclusive Control clients can connect to the same device.")
                .build();

        auto dropOthersProp =
            BoolPropertyBuilder("ExclusiveControlDropOthers", false)
                .setDescription("If enabled, when connecting as an Exclusive Control client, any existing Control clients will be disconnected.")
                .build();

        obj.addProperty(clientTypeProp);
        obj.addProperty(dropOthersProp);
    }

    static ClientType IntToClientType(Int value)
    {
        switch (value)
        {
            case static_cast<Int>(ClientType::Control):
                return ClientType::Control;
            case static_cast<Int>(ClientType::ExclusiveControl):
                return ClientType::ExclusiveControl;
            case static_cast<Int>(ClientType::ViewOnly):
                return ClientType::ViewOnly;
        }

        DAQ_THROW_EXCEPTION(InvalidValueException, "Client type value invalid");
    }

    static StringPtr ClientTypeToString(ClientType value)
    {
        switch (value)
        {
            case ClientType::Control:
                return String("Control");
            case ClientType::ExclusiveControl:
                return String("ExclusiveControl");
            case ClientType::ViewOnly:
                return String("ViewOnly");
        }

        return String("");
    }
};

END_NAMESPACE_OPENDAQ
