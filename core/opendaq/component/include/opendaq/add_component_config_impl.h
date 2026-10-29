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
#include <coreobjects/property_object_impl.h>
#include <coreobjects/property_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/validation.h>
#include <fmt/format.h>

BEGIN_NAMESPACE_OPENDAQ

// Shared by the add-device and add-streaming configs, which differ only in their interface and serialize id.
template <typename Intf>
class GenericAddComponentConfigImpl : public GenericPropertyObjectImpl<Intf>
{
public:
    using Super = GenericPropertyObjectImpl<Intf>;

    // The first transport starts out selected
    explicit GenericAddComponentConfigImpl(const ListPtr<IString>& transports = nullptr);

    ErrCode INTERFACE_FUNC getAvailableTransports(IList** transports) override;
    ErrCode INTERFACE_FUNC setTransport(IString* transport) override;
    ErrCode INTERFACE_FUNC getTransport(IString** transport) override;

    // An object-type property value must report IPropertyObject as its first interface
    ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) override;

protected:
    static constexpr const char* TransportPropertyName = "Transport";
};

template <typename Intf>
GenericAddComponentConfigImpl<Intf>::GenericAddComponentConfigImpl(const ListPtr<IString>& transports)
    : Super()
{
    // A selection property cannot be empty, so a type without transports gets no "Transport" property
    if (transports.assigned() && transports.getCount() > 0)
        Super::addProperty(SelectionProperty(TransportPropertyName, transports, 0));
}

template <typename Intf>
ErrCode GenericAddComponentConfigImpl<Intf>::getAvailableTransports(IList** transports)
{
    OPENDAQ_PARAM_NOT_NULL(transports);

    return daqTry([&]
    {
        auto names = List<IString>();
        if (this->objPtr.hasProperty(TransportPropertyName))
        {
            const ListPtr<IString> values = this->objPtr.getProperty(TransportPropertyName).getSelectionValues();
            for (const auto& name : values)
                names.pushBack(name);
        }

        *transports = names.detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename Intf>
ErrCode GenericAddComponentConfigImpl<Intf>::setTransport(IString* transport)
{
    OPENDAQ_PARAM_NOT_NULL(transport);

    return daqTry([&]
    {
        const StringPtr name = StringPtr::Borrow(transport);
        if (this->objPtr.hasProperty(TransportPropertyName))
        {
            const ListPtr<IString> values = this->objPtr.getProperty(TransportPropertyName).getSelectionValues();
            for (const auto& value : values)
            {
                if (value == name)
                    return this->setPropertySelectionValue(String(TransportPropertyName), transport);
            }
        }

        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"("{}" is not one of the available transports)", name));
    });
}

template <typename Intf>
ErrCode GenericAddComponentConfigImpl<Intf>::getTransport(IString** transport)
{
    OPENDAQ_PARAM_NOT_NULL(transport);

    return daqTry([&]
    {
        if (!this->objPtr.hasProperty(TransportPropertyName))
        {
            *transport = nullptr;
            return OPENDAQ_SUCCESS;
        }

        StringPtr name = this->objPtr.getPropertySelectionValue(TransportPropertyName);
        *transport = name.detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename Intf>
ErrCode GenericAddComponentConfigImpl<Intf>::getInterfaceIds(SizeT* idCount, IntfID** ids)
{
    OPENDAQ_PARAM_NOT_NULL(idCount);

    *idCount = Super::InterfaceIds::Count() + 1;
    if (ids == nullptr)
        return OPENDAQ_SUCCESS;

    **ids = IPropertyObject::Id;
    (*ids)++;

    Super::InterfaceIds::AddInterfaceIds(*ids);
    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
