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
#include <opendaq/transport_option.h>
#include <opendaq/transport_option_factory.h>
#include <coreobjects/property_object_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coretypes/listobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

// The transport option keeps its fields as the default values of its own properties and is frozen right away.
// It has no serialize id of its own: it is serialized as a plain Property object, which any peer reads.
class TransportOptionImpl final : public GenericPropertyObjectImpl<ITransportOption>
{
public:
    using Super = GenericPropertyObjectImpl<ITransportOption>;

    static constexpr const char* NamePropertyName = "Name";
    static constexpr const char* PortPropertyName = "Port";
    static constexpr const char* ProtectionPropertyName = "Protection";
    static constexpr const char* CertificateRequiredPropertyName = "CertificateRequired";

    TransportOptionImpl(const StringPtr& name, const IntegerPtr& port, TransportProtection protection, Bool certificateRequired);

    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getPort(IInteger** port) override;
    ErrCode INTERFACE_FUNC getProtection(TransportProtection* protection) override;
    ErrCode INTERFACE_FUNC getCertificateRequired(Bool* required) override;

    // An object-type property value must report IPropertyObject as its first interface
    ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) override;

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;
};

/*!
 * @brief Gets the transport option an object stands for.
 * @param obj A transport option, or the plain Property object a transport option was deserialized into.
 * @throws InvalidParameterException if `obj` is neither, i.e. lacks one of the option's properties.
 *
 * Component types use it to restore the options they read back from their serialized form. The header is private to
 * the SDK; the public server capability header restores its options on its own.
 */
inline TransportOptionPtr TransportOptionFromPropertyObject(const PropertyObjectPtr& obj)
{
    if (!obj.assigned())
        DAQ_THROW_EXCEPTION(ArgumentNullException, "The transport option object is not assigned");

    if (const auto option = obj.asPtrOrNull<ITransportOption, TransportOptionPtr>(true); option.assigned())
        return option;

    const auto hasProperty = [&obj](const char* name, CoreType type)
    {
        return obj.hasProperty(name) && obj.getProperty(name).getValueType() == type;
    };

    if (!hasProperty(TransportOptionImpl::NamePropertyName, ctString) || !hasProperty(TransportOptionImpl::PortPropertyName, ctInt) ||
        !hasProperty(TransportOptionImpl::ProtectionPropertyName, ctInt) ||
        !hasProperty(TransportOptionImpl::CertificateRequiredPropertyName, ctBool))
    {
        DAQ_THROW_EXCEPTION(InvalidParameterException, "The object is not a transport option");
    }

    const StringPtr name = obj.getPropertyValue(TransportOptionImpl::NamePropertyName);
    const Int port = obj.getPropertyValue(TransportOptionImpl::PortPropertyName);
    const Int protection = obj.getPropertyValue(TransportOptionImpl::ProtectionPropertyName);
    const Bool certificateRequired = obj.getPropertyValue(TransportOptionImpl::CertificateRequiredPropertyName);
    return TransportOption(name, port, static_cast<TransportProtection>(protection), certificateRequired);
}

/*!
 * @brief Gets the transport options a list of objects stands for, in the same order.
 * @param objects Transport options or the plain Property objects they were deserialized into; may be unassigned.
 * @throws InvalidParameterException if an element is neither.
 */
inline ListPtr<ITransportOption> TransportOptionsFromPropertyObjects(const ListPtr<IBaseObject>& objects)
{
    auto options = List<ITransportOption>();
    if (!objects.assigned())
        return options;

    for (const auto& obj : objects)
    {
        const auto propertyObject = obj.asPtrOrNull<IPropertyObject, PropertyObjectPtr>();
        if (!propertyObject.assigned())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "The object is not a transport option");

        options.pushBack(TransportOptionFromPropertyObject(propertyObject));
    }

    return options;
}

/*!
 * @brief Gets the names of transport options, in the same order.
 */
inline ListPtr<IString> TransportOptionNames(const ListPtr<ITransportOption>& options)
{
    auto names = List<IString>();
    if (options.assigned())
    {
        for (const auto& option : options)
            names.pushBack(option.getName());
    }

    return names;
}

/*!
 * @brief Finds a transport option by name.
 * @returns The option, or unassigned if `options` has none of that name.
 */
inline TransportOptionPtr FindTransportOption(const ListPtr<ITransportOption>& options, const StringPtr& name)
{
    if (options.assigned() && name.assigned())
    {
        for (const auto& option : options)
        {
            if (option.getName() == name)
                return option;
        }
    }

    return nullptr;
}

END_NAMESPACE_OPENDAQ
