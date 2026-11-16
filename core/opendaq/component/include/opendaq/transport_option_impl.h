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
#include <opendaq/component_type.h>
#include <coreobjects/property_object_impl.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coretypes/dictobject_factory.h>
#include <fmt/format.h>

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
 * @brief Gets the transport options a dictionary of objects stands for, in the same order.
 * @param objects Transport options or the plain Property objects they were deserialized into, keyed by their own
 * names; may be unassigned.
 * @returns The options keyed by their names, frozen.
 * @throws InvalidParameterException if a value is neither, or a key is not the name of its option.
 */
inline DictPtr<IString, ITransportOption> TransportOptionsFromPropertyObjects(const DictPtr<IBaseObject, IBaseObject>& objects)
{
    auto options = Dict<IString, ITransportOption>();
    if (objects.assigned())
    {
        for (const auto& [key, obj] : objects)
        {
            const auto propertyObject = obj.asPtrOrNull<IPropertyObject, PropertyObjectPtr>();
            if (!propertyObject.assigned())
                DAQ_THROW_EXCEPTION(InvalidParameterException, "The object is not a transport option");

            const auto option = TransportOptionFromPropertyObject(propertyObject);
            const auto name = key.asPtrOrNull<IString, StringPtr>();
            if (!name.assigned() || name != option.getName())
                DAQ_THROW_EXCEPTION(InvalidParameterException, R"(The transport option "{}" is not keyed by its name)", option.getName());

            options.set(name, option);
        }
    }

    options.freeze();
    return options;
}

/*!
 * @brief Gets transport options keyed by their names, in the given order.
 */
inline DictPtr<IString, ITransportOption> TransportOptionsByName(std::initializer_list<TransportOptionPtr> options)
{
    auto dict = Dict<IString, ITransportOption>();
    for (const auto& option : options)
        dict.set(option.getName(), option);

    return dict;
}

/*!
 * @brief Gets a frozen copy of transport options keyed by their names, in the same order.
 */
inline DictPtr<IString, ITransportOption> FrozenTransportOptions(const DictPtr<IString, ITransportOption>& options)
{
    auto copy = Dict<IString, ITransportOption>();
    if (options.assigned())
    {
        for (const auto& [name, option] : options)
            copy.set(name, option);
    }

    copy.freeze();
    return copy;
}

/*!
 * @brief Gets the first transport option, the default one.
 * @returns The option, or unassigned if there are none.
 */
inline TransportOptionPtr FirstTransportOption(const DictPtr<IString, ITransportOption>& options)
{
    if (options.assigned())
    {
        for (const auto& [name, option] : options)
            return option;
    }

    return nullptr;
}

/*!
 * @brief Adds the properties a module connects by to the config a device or streaming is created with.
 * @param config The type config.
 * @param options The type's transport options; nothing is added when there are none.
 *
 * Adds TransportOptionConfigProperty, set to the name of the first option, and PortConfigProperty, set to its port.
 * A property the config already has is kept.
 */
inline void AddTransportSelectionProperties(const PropertyObjectPtr& config, const DictPtr<IString, ITransportOption>& options)
{
    const TransportOptionPtr defaultOption = FirstTransportOption(options);
    if (!config.assigned() || !defaultOption.assigned())
        return;

    if (!config.hasProperty(TransportOptionConfigProperty))
        config.addProperty(StringProperty(TransportOptionConfigProperty, defaultOption.getName()));
    if (!config.hasProperty(PortConfigProperty))
        config.addProperty(IntProperty(PortConfigProperty, defaultOption.getPort()));
}

/*!
 * @brief Gets the transport options of a device or streaming type.
 * @returns The options keyed by their names, or an empty dictionary if the type lists none or does not implement them.
 */
inline DictPtr<IString, ITransportOption> ComponentTypeTransportOptions(IComponentType* type)
{
    DictPtr<IString, ITransportOption> options;
    if (type == nullptr || OPENDAQ_FAILED(type->getAvailableTransportOptions(&options)))
        daqClearErrorInfo();

    return options.assigned() ? options : Dict<IString, ITransportOption>();
}

/*!
 * @brief Gets the expression of the port of the selected transport option, for the default value of the port property of
 * an add-device or add-streaming config.
 * @param options The options keyed by their names; none of the names may contain a quote.
 * @param selectionProperty The name of the string property that names the selected option.
 * @returns A `switch` over the option names with their ports, the port of the first option for a name it does not list.
 */
inline StringPtr SelectedTransportOptionPortEval(const DictPtr<IString, ITransportOption>& options, const std::string& selectionProperty)
{
    const TransportOptionPtr defaultOption = FirstTransportOption(options);
    if (!defaultOption.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "There are no transport options to select");

    std::string eval = "switch($" + selectionProperty;
    for (const auto& [name, option] : options)
    {
        const std::string nameStr = name;
        if (nameStr.find('\'') != std::string::npos)
            DAQ_THROW_EXCEPTION(InvalidParameterException, R"(The transport option name "{}" contains a quote)", nameStr);

        eval += fmt::format(", '{}', {}", nameStr, static_cast<Int>(option.getPort()));
    }

    return eval + fmt::format(", {})", static_cast<Int>(defaultOption.getPort()));
}

END_NAMESPACE_OPENDAQ
