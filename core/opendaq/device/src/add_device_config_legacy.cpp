#include <opendaq/add_device_config_legacy.h>
#include <opendaq/client_type.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

BEGIN_NAMESPACE_OPENDAQ

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

}

extern "C" PUBLIC_EXPORT ErrCode daqCreateLegacyGeneralConfig(IPropertyObject** general, Bool scanOnAdd)
{
    OPENDAQ_PARAM_NOT_NULL(general);

    return daqTry([&] { *general = createGeneralConfig(scanOnAdd).detach(); });
}

END_NAMESPACE_OPENDAQ
