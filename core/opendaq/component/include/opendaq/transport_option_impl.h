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
#include <coretypes/impl.h>
#include <coretypes/serializable.h>
#include <coretypes/serialized_object.h>
#include <coretypes/dictobject_factory.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/property_factory.h>
#include <fmt/format.h>

BEGIN_NAMESPACE_OPENDAQ

// An immutable transport option, serialized as a tagged object with its protection as a string. A server capability
// does not hold it as it is: it keeps each option as a plain Property object, which any peer reads.
class TransportOptionImpl final : public ImplementationOf<ITransportOption, ISerializable>
{
public:
    TransportOptionImpl(const StringPtr& id, const IntegerPtr& port, TransportProtection protection, Bool certificateRequired);

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC getPort(IInteger** port) override;
    ErrCode INTERFACE_FUNC getProtection(TransportProtection* protection) override;
    ErrCode INTERFACE_FUNC getCertificateRequired(Bool* required) override;

    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    StringPtr id;
    IntegerPtr port;
    TransportProtection protection;
    Bool certificateRequired;
};

/*!
 * @brief Gets transport options keyed by their ids, in the given order.
 */
inline DictPtr<IString, ITransportOption> TransportOptionsById(std::initializer_list<TransportOptionPtr> options)
{
    auto dict = Dict<IString, ITransportOption>();
    for (const auto& option : options)
        dict.set(option.getId(), option);

    return dict;
}

/*!
 * @brief Gets a frozen copy of transport options keyed by their ids, in the same order.
 * @param options The options, for example as deserialized; may be unassigned.
 * @throws InvalidTypeException if a key is not a string or a value is not a transport option.
 * @throws InvalidValueException if a key is not the id of its option.
 */
inline DictPtr<IString, ITransportOption> FrozenTransportOptions(const DictPtr<IBaseObject, IBaseObject>& options)
{
    auto copy = Dict<IString, ITransportOption>();
    if (options.assigned())
    {
        for (const auto& [key, item] : options)
        {
            const auto id = key.asPtrOrNull<IString, StringPtr>();
            const auto option = item.asPtrOrNull<ITransportOption, TransportOptionPtr>();
            if (!id.assigned() || !option.assigned())
                DAQ_THROW_EXCEPTION(InvalidTypeException, "Transport options must be ITransportOption objects keyed by their ids");

            if (id != option.getId())
                DAQ_THROW_EXCEPTION(InvalidValueException, R"(Transport option "{}" is keyed by "{}")", option.getId(), id);

            copy.set(id, option);
        }
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
        for (const auto& [id, option] : options)
            return option;
    }

    return nullptr;
}

/*!
 * @brief Adds the properties a module connects by to the config a device or streaming is created with.
 * @param config The type config.
 * @param options The type's transport options; nothing is added when there are none.
 *
 * Adds TransportOptionConfigProperty, set to the id of the first option, and PortConfigProperty, set to its port.
 * A property the config already has is kept.
 */
inline void AddTransportSelectionProperties(const PropertyObjectPtr& config, const DictPtr<IString, ITransportOption>& options)
{
    const TransportOptionPtr defaultOption = FirstTransportOption(options);
    if (!config.assigned() || !defaultOption.assigned())
        return;

    if (!config.hasProperty(TransportOptionConfigProperty))
        config.addProperty(StringProperty(TransportOptionConfigProperty, defaultOption.getId()));
    if (!config.hasProperty(PortConfigProperty))
        config.addProperty(IntProperty(PortConfigProperty, defaultOption.getPort()));
}

/*!
 * @brief Gets the transport options of a device or streaming type.
 * @returns The options keyed by their ids, or an empty dictionary if the type lists none or does not implement them.
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
 * @param options The options keyed by their ids; none of the ids may contain a quote.
 * @param selectionProperty The name of the string property that holds the id of the selected option.
 * @returns A `switch` over the option ids with their ports, the port of the first option for an id it does not list.
 */
inline StringPtr SelectedTransportOptionPortEval(const DictPtr<IString, ITransportOption>& options, const std::string& selectionProperty)
{
    const TransportOptionPtr defaultOption = FirstTransportOption(options);
    if (!defaultOption.assigned())
        DAQ_THROW_EXCEPTION(InvalidParameterException, "There are no transport options to select");

    std::string eval = "switch($" + selectionProperty;
    for (const auto& [id, option] : options)
    {
        const std::string idStr = id;
        if (idStr.find('\'') != std::string::npos)
            DAQ_THROW_EXCEPTION(InvalidParameterException, R"(The transport option id "{}" contains a quote)", idStr);

        eval += fmt::format(", '{}', {}", idStr, static_cast<Int>(option.getPort()));
    }

    return eval + fmt::format(", {})", static_cast<Int>(defaultOption.getPort()));
}

END_NAMESPACE_OPENDAQ
