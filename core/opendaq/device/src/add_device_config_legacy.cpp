#include <opendaq/add_device_config_legacy.h>
#include <opendaq/add_component_config_factory.h>
#include <opendaq/streaming_type_utils.h>
#include <opendaq/client_type.h>
#include <opendaq/custom_log.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{

static void copyValues(const PropertyObjectPtr& target, const PropertyObjectPtr& source)
{
    for (const auto& prop : target.getAllProperties())
    {
        const auto name = prop.getName();
        if (!source.hasProperty(name))
            continue;

        const auto sourceValue = source.getPropertyValue(name);
        if (target.getPropertyValue(name) == sourceValue)
            continue;

        const auto valueType = prop.getValueType();
        if (valueType == ctObject)
            copyValues(target.getPropertyValue(name), sourceValue);
        else if (valueType == ctFunc || valueType == ctProc)
            target.asPtr<IPropertyObjectProtected>().setProtectedPropertyValue(name, sourceValue);
        else
            target.setPropertyValue(name, sourceValue);
    }
}

}

namespace
{

PropertyObjectPtr createGeneralConfig(Bool scanOnAdd)
{
    auto obj = PropertyObject();

    const auto streamingConnectionHeuristicProp = SelectionProperty(
        LegacyAddDeviceConfig::StreamingConnectionHeuristic,
        List<IString>(LegacyAddDeviceConfig::MinConnections, LegacyAddDeviceConfig::MinHops, LegacyAddDeviceConfig::NotConnected),
        0);
    obj.addProperty(streamingConnectionHeuristicProp);

    obj.addProperty(
        ListProperty(LegacyAddDeviceConfig::PrioritizedStreamingProtocols, LegacyAddDeviceConfig::defaultPrioritizedStreamingProtocols()));

    obj.addProperty(ListProperty(LegacyAddDeviceConfig::AllowedStreamingProtocols, List<IString>()));

    obj.addProperty(BoolProperty(LegacyAddDeviceConfig::AutomaticallyConnectStreaming, true));

    obj.addProperty(
        BoolPropertyBuilder(LegacyAddDeviceConfig::ScanOnAdd, scanOnAdd)
            .setDescription("Scans the network when adding the device; when off, a daq:// connection string needs an earlier scan")
            .build());

    obj.addProperty(StringProperty(LegacyAddDeviceConfig::Username, ""));
    obj.addProperty(StringProperty(LegacyAddDeviceConfig::Password, ""));

    ClientTypeTools::DefineConfigProperties(obj);

    obj.addProperty(
        StringPropertyBuilder(LegacyAddDeviceConfig::PrimaryAddressType, "")
            .setDescription(
                "Specifies the primary address type for establishing configuration and streaming protocols connections "
                "while using smart connection string with \"daq://\" prefix. "
                "Acceptable values are \"IPv4\" or \"IPv6\"; if left blank, any address type may be used. "
                "If no addresses of the specified type are available, the first available address of the alternate type will be used.")
            .build());

    return obj.detach();
}

AddDeviceConfigPtr fromLegacy(const PropertyObjectPtr& legacyConfig,
                              const DeviceTypePtr& deviceType,
                              const DictPtr<IString, IStreamingType>& streamingTypes,
                              Bool scanOnAdd,
                              const LoggerComponentPtr& loggerComponent)
{
    if (!loggerComponent.assigned())
    {
        DAQ_THROW_EXCEPTION(ArgumentNullException, "The logger component is null");
    }

    const auto sectionOf = [](const PropertyObjectPtr& obj, const StringPtr& name)
    {
        return obj.hasProperty(name) ? PropertyObjectPtr(obj.getPropertyValue(name)) : PropertyObject();
    };

    const auto general = createGeneralConfig(scanOnAdd);
    detail::copyValues(general, sectionOf(legacyConfig, LegacyAddDeviceConfig::GeneralSection));

    const auto streamingSection = sectionOf(legacyConfig, LegacyAddDeviceConfig::StreamingSection);
    const auto loadedTypes = streamingTypes.assigned() ? streamingTypes : Dict<IString, IStreamingType>();

    const bool hasStreamingSection = legacyConfig.hasProperty(LegacyAddDeviceConfig::StreamingSection);
    auto candidates = Dict<IString, IStreamingType>();
    for (const auto& [typeId, streamingType] : loadedTypes)
        if (!hasStreamingSection || streamingSection.hasProperty(typeId))
            candidates.set(typeId, streamingType);

    auto config = AddDeviceConfig(deviceType, getListableStreamingTypes(candidates, loggerComponent), scanOnAdd);
    config.setScanOnAdd(general.getPropertyValue(LegacyAddDeviceConfig::ScanOnAdd));
    const auto options = config.getStreamingOptions();
    const auto streamingConfigs = options.assigned() ? options.getStreamingConfigs() : Dict<IString, IAddStreamingConfig>();

    const auto translateTypeConfig = [&general](const PropertyObjectPtr& target, const PropertyObjectPtr& source)
    {
        try
        {
            detail::copyValues(target, source);
        }
        catch (const DaqException& e)
        {
            DAQ_THROW_EXCEPTION(InvalidValueException, "{}", e.what());
        }

        for (const auto name : {LegacyAddDeviceConfig::Username,
                                LegacyAddDeviceConfig::Password,
                                LegacyAddDeviceConfig::ClientType,
                                LegacyAddDeviceConfig::ExclusiveControlDropOthers})
            if (target.hasProperty(name) && target.getPropertyValue(name) == general.getProperty(name).getDefaultValue())
                target.setPropertyValue(name, general.getPropertyValue(name));
    };

    const auto deviceSection = sectionOf(legacyConfig, LegacyAddDeviceConfig::DeviceSection);
    translateTypeConfig(config.getTypeConfig(), sectionOf(deviceSection, deviceType.getId()));
    for (const auto& [typeId, streamingConfig] : streamingConfigs)
        translateTypeConfig(streamingConfig.getTypeConfig(), sectionOf(streamingSection, typeId));

    const StringPtr addressType = general.getPropertyValue(LegacyAddDeviceConfig::PrimaryAddressType);
    if (addressType == LegacyAddDeviceConfig::IPv6)
        config.setPrimaryAddressType(AddressType::IPv6);
    else if (addressType.getLength() > 0 && addressType != LegacyAddDeviceConfig::IPv4)
    {
        LOG_W(R"(Primary address type "{}" is unknown, IPv4 is used)", addressType);
    }

    if (!options.assigned())
        return config;

    const StringPtr heuristic = general.getPropertySelectionValue(LegacyAddDeviceConfig::StreamingConnectionHeuristic);
    options.setStreamingMode((heuristic == LegacyAddDeviceConfig::MinHops) ? StreamingMode::MinHops : StreamingMode::MinConnections);
    if (heuristic == LegacyAddDeviceConfig::NotConnected ||
        !static_cast<Bool>(general.getPropertyValue(LegacyAddDeviceConfig::AutomaticallyConnectStreaming)))
        return config;

    const ListPtr<IString> prioritized = general.getPropertyValue(LegacyAddDeviceConfig::PrioritizedStreamingProtocols);
    const ListPtr<IString> allowed = general.getPropertyValue(LegacyAddDeviceConfig::AllowedStreamingProtocols);
    const auto isAllowed = [&allowed](const StringPtr& protocol)
    {
        return (allowed.getCount() == 0) || (std::find(allowed.begin(), allowed.end(), protocol) != allowed.end());
    };

    const auto pick = std::find_if(prioritized.begin(), prioritized.end(), isAllowed);
    if (pick == prioritized.end())
        return config;

    if (!streamingConfigs.hasKey(*pick))
    {
        LOG_W(R"(Streaming protocol "{}" is not a listed streaming type of device type "{}", no streaming type is selected)",
              *pick,
              deviceType.getId());
        return config;
    }

    options.setStreamingType(*pick);
    return config;
}

PropertyObjectPtr toLegacy(const AddDeviceConfigPtr& config)
{
    const auto cloneTypeConfig = [](const PropertyObjectPtr& typeConfig) -> PropertyObjectPtr
    {
        return typeConfig.asPtr<IPropertyObjectInternal>().clone();
    };

    auto deviceSection = PropertyObject();
    deviceSection.addProperty(ObjectProperty(config.getDeviceType().getId(), cloneTypeConfig(config.getTypeConfig())));

    auto streamingSection = PropertyObject();
    auto general = createGeneralConfig(config.getScanOnAdd());

    const auto addressType =
        (config.getPrimaryAddressType() == AddressType::IPv6) ? LegacyAddDeviceConfig::IPv6 : LegacyAddDeviceConfig::IPv4;
    general.setPropertyValue(LegacyAddDeviceConfig::PrimaryAddressType, addressType);

    const auto options = config.getStreamingOptions();
    if (options.assigned())
    {
        for (const auto& [typeId, streamingConfig] : options.getStreamingConfigs())
            streamingSection.addProperty(ObjectProperty(typeId, cloneTypeConfig(streamingConfig.getTypeConfig())));

        const auto selected = options.getStreamingConfig();
        if (selected.assigned())
            general.setPropertyValue(LegacyAddDeviceConfig::PrioritizedStreamingProtocols,
                                     List<IString>(selected.getStreamingType().getId()));
        else
            general.setPropertyValue(LegacyAddDeviceConfig::AutomaticallyConnectStreaming, False);

        const auto heuristic =
            (options.getStreamingMode() == StreamingMode::MinHops) ? LegacyAddDeviceConfig::MinHops : LegacyAddDeviceConfig::MinConnections;
        general.setPropertySelectionValue(LegacyAddDeviceConfig::StreamingConnectionHeuristic, heuristic);
    }
    else if (const auto kind = config.getDeviceType().getDeviceKind(); kind != DeviceKind::Adapter && kind != DeviceKind::Root)
        general.setPropertyValue(LegacyAddDeviceConfig::AutomaticallyConnectStreaming, False);

    auto legacyConfig = PropertyObject();
    legacyConfig.addProperty(ObjectProperty(LegacyAddDeviceConfig::DeviceSection, deviceSection.detach()));
    legacyConfig.addProperty(ObjectProperty(LegacyAddDeviceConfig::StreamingSection, streamingSection.detach()));
    legacyConfig.addProperty(ObjectProperty(LegacyAddDeviceConfig::GeneralSection, general.detach()));
    return legacyConfig.detach();
}

}

extern "C" PUBLIC_EXPORT ErrCode daqCreateLegacyGeneralConfig(IPropertyObject** general, Bool scanOnAdd)
{
    OPENDAQ_PARAM_NOT_NULL(general);

    return daqTry([&] { *general = createGeneralConfig(scanOnAdd).detach(); });
}

extern "C" PUBLIC_EXPORT ErrCode daqFromLegacyAddDeviceConfig(IAddDeviceConfig** config,
                                                              IPropertyObject* legacyConfig,
                                                              IDeviceType* deviceType,
                                                              IDict* streamingTypes,
                                                              Bool scanOnAdd,
                                                              ILoggerComponent* loggerComponent)
{
    OPENDAQ_PARAM_NOT_NULL(config);

    return daqTry([&] { *config = fromLegacy(legacyConfig, deviceType, streamingTypes, scanOnAdd, loggerComponent).detach(); });
}

extern "C" PUBLIC_EXPORT ErrCode daqToLegacyAddDeviceConfig(IPropertyObject** legacyConfig, IAddDeviceConfig* config)
{
    OPENDAQ_PARAM_NOT_NULL(legacyConfig);

    return daqTry([&] { *legacyConfig = toLegacy(config).detach(); });
}

END_NAMESPACE_OPENDAQ
