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
 * Owners of transport options (component types, server capabilities) use it to restore the options they read back
 * from their serialized form.
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

END_NAMESPACE_OPENDAQ
