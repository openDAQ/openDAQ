#include <opendaq/add_device_config_legacy.h>
#include <opendaq/address_info_utils.h>
#include <opendaq/add_component_config_factory.h>
#include <opendaq/streaming_type_utils.h>
#include <opendaq/client_type.h>
#include <opendaq/custom_log.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coretypes/listobject_factory.h>
#include <algorithm>

BEGIN_NAMESPACE_OPENDAQ

namespace LegacyAddDeviceConfig
{
static constexpr const char* DeviceSection = "Device";
static constexpr const char* StreamingSection = "Streaming";
static constexpr const char* GeneralSection = "General";

static constexpr const char* StreamingConnectionHeuristic = "StreamingConnectionHeuristic";
static constexpr const char* MinConnections = "MinConnections";
static constexpr const char* MinHops = "MinHops";
static constexpr const char* NotConnected = "NotConnected";

static constexpr const char* PrioritizedStreamingProtocols = "PrioritizedStreamingProtocols";
static constexpr const char* AllowedStreamingProtocols = "AllowedStreamingProtocols";
static constexpr const char* AutomaticallyConnectStreaming = "AutomaticallyConnectStreaming";
static constexpr const char* ScanOnAdd = "ScanOnAdd";
static constexpr const char* Username = "Username";
static constexpr const char* Password = "Password";
static constexpr const char* ClientType = "ClientType";
static constexpr const char* ExclusiveControlDropOthers = "ExclusiveControlDropOthers";
static constexpr const char* PrimaryAddressType = "PrimaryAddressType";
static constexpr const char* IPv4 = "IPv4";
static constexpr const char* IPv6 = "IPv6";

static constexpr const char* NativeStreamingProtocolId = "OpenDAQNativeStreaming";
static constexpr const char* LtStreamingProtocolId = "OpenDAQLTStreaming";

static ListPtr<IString> defaultPrioritizedStreamingProtocols()
{
    return List<IString>(NativeStreamingProtocolId, LtStreamingProtocolId);
}
}

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

static PropertyObjectPtr sectionOf(const PropertyObjectPtr& obj,
                                   const StringPtr& name,
                                   const PropertyObjectPtr& fallback = PropertyObject())
{
    return obj.hasProperty(name) ? PropertyObjectPtr(obj.getPropertyValue(name)) : fallback;
}

static PropertyObjectPtr createGeneral(const PropertyObjectPtr& source, Bool scanOnAdd)
{
    auto general = createLegacyGeneralConfig(scanOnAdd);
    copyValues(general, source);
    return general;
}

static PropertyObjectPtr generalOf(const PropertyObjectPtr& legacyConfig, Bool scanOnAdd)
{
    return createGeneral(sectionOf(legacyConfig, LegacyAddDeviceConfig::GeneralSection, legacyConfig), scanOnAdd);
}

static void translateTypeConfig(const PropertyObjectPtr& target, const PropertyObjectPtr& source, const PropertyObjectPtr& general)
{
    try
    {
        copyValues(target, source);
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
}

}

PropertyObjectPtr createLegacyGeneralConfig(Bool scanOnAdd)
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

PropertyObjectPtr createLegacyAddDeviceConfig(const DictPtr<IString, IDeviceType>& deviceTypes,
                                              const DictPtr<IString, IStreamingType>& streamingTypes,
                                              Bool scanOnAdd)
{
    auto deviceSection = PropertyObject();
    for (const auto& [id, type] : deviceTypes)
        deviceSection.addProperty(ObjectProperty(id, type.createDefaultConfig()));

    auto streamingSection = PropertyObject();
    for (const auto& [id, type] : streamingTypes)
        streamingSection.addProperty(ObjectProperty(id, type.createDefaultConfig()));

    auto config = PropertyObject();
    config.addProperty(ObjectProperty(LegacyAddDeviceConfig::DeviceSection, deviceSection.detach()));
    config.addProperty(ObjectProperty(LegacyAddDeviceConfig::StreamingSection, streamingSection.detach()));
    config.addProperty(ObjectProperty(LegacyAddDeviceConfig::GeneralSection, createLegacyGeneralConfig(scanOnAdd).detach()));
    return config.detach();
}

AddDeviceConfigPtr fromLegacyAddDeviceConfig(const PropertyObjectPtr& input,
                                             const DeviceTypePtr& deviceType,
                                             const DictPtr<IString, IStreamingType>& streamingTypes,
                                             Bool scanOnAdd,
                                             const LoggerComponentPtr& loggerComponent)
{
    const auto legacyConfig = input.assigned() ? input : PropertyObject();
    const auto general = detail::generalOf(legacyConfig, scanOnAdd);
    const auto streamingSection = detail::sectionOf(legacyConfig, LegacyAddDeviceConfig::StreamingSection);
    const auto loadedTypes = streamingTypes.assigned() ? streamingTypes : Dict<IString, IStreamingType>();

    const bool hasStreamingSection = legacyConfig.hasProperty(LegacyAddDeviceConfig::StreamingSection);
    auto candidates = Dict<IString, IStreamingType>();
    for (const auto& [typeId, streamingType] : loadedTypes)
        if (!hasStreamingSection || streamingSection.hasProperty(typeId))
            candidates.set(typeId, streamingType);

    auto config = AddDeviceConfig(deviceType, getListableStreamingTypes(candidates, loggerComponent), scanOnAdd);
    config.setScanOnAdd(general.getPropertyValue(LegacyAddDeviceConfig::ScanOnAdd));
    const auto options = config.getStreamingOptions();
    const auto streamingConfigs = options.assigned() ? options.getAvailableStreamingConfigs() : Dict<IString, IAddStreamingConfig>();

    const auto deviceSection = detail::sectionOf(legacyConfig, LegacyAddDeviceConfig::DeviceSection);
    detail::translateTypeConfig(config.getTypeConfig(), detail::sectionOf(deviceSection, deviceType.getId(), legacyConfig), general);
    for (const auto& [typeId, streamingConfig] : streamingConfigs)
        detail::translateTypeConfig(streamingConfig.getTypeConfig(), detail::sectionOf(streamingSection, typeId), general);

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

Bool getLegacyScanOnAdd(const PropertyObjectPtr& legacyConfig, Bool scanOnAdd)
{
    const auto general = detail::generalOf(legacyConfig.assigned() ? legacyConfig : PropertyObject(), scanOnAdd);
    return general.getPropertyValue(LegacyAddDeviceConfig::ScanOnAdd);
}

PropertyObjectPtr fromLegacyAddStreamingConfig(const PropertyObjectPtr& config, const StreamingTypePtr& streamingType)
{
    if (!config.assigned() || !config.hasProperty(LegacyAddDeviceConfig::DeviceSection) ||
        !config.hasProperty(LegacyAddDeviceConfig::StreamingSection) || !config.hasProperty(LegacyAddDeviceConfig::GeneralSection))
        return config;

    const auto general = detail::createGeneral(config.getPropertyValue(LegacyAddDeviceConfig::GeneralSection), False);
    const PropertyObjectPtr streamingSection = config.getPropertyValue(LegacyAddDeviceConfig::StreamingSection);

    auto typeConfig = streamingType.createDefaultConfig();
    detail::translateTypeConfig(typeConfig, detail::sectionOf(streamingSection, streamingType.getId()), general);
    return typeConfig;
}

PropertyObjectPtr toLegacyAddDeviceConfig(const AddDeviceConfigPtr& config)
{
    const auto cloneTypeConfig = [](const PropertyObjectPtr& typeConfig) -> PropertyObjectPtr
    {
        return typeConfig.asPtr<IPropertyObjectInternal>().clone();
    };

    auto deviceSection = PropertyObject();
    deviceSection.addProperty(ObjectProperty(config.getDeviceType().getId(), cloneTypeConfig(config.getTypeConfig())));

    auto streamingSection = PropertyObject();
    auto general = createLegacyGeneralConfig(config.getScanOnAdd());

    const auto addressType = getAddressTypeName(config.getPrimaryAddressType());
    general.setPropertyValue(LegacyAddDeviceConfig::PrimaryAddressType, addressType);

    const auto options = config.getStreamingOptions();
    if (options.assigned())
    {
        for (const auto& [typeId, streamingConfig] : options.getAvailableStreamingConfigs())
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

END_NAMESPACE_OPENDAQ
