#include <discovery_common/daq_discovery_common.h>

BEGIN_NAMESPACE_DISCOVERY_COMMON

void IpModificationUtils::encodeIpConfiguration(const PropertyObjectPtr& config, TxtProperties& props)
{
    const bool dhcp4Mode = config.getPropertyValue("dhcp4");
    props["dhcp4"] = dhcp4Mode ? "1" : "0";
    ListPtr<IString> addresses4List = config.getPropertyValue("addresses4");
    std::string addresses4String = "";
    for (const auto& addr : addresses4List)
        addresses4String += addr.toStdString() + ";";
    props["addresses4"] = addresses4String;
    StringPtr gateway4 = config.getPropertyValue("gateway4");
    props["gateway4"] = gateway4.toStdString();

    const bool dhcp6Mode = config.getPropertyValue("dhcp6");
    props["dhcp6"] = dhcp6Mode ? "1" : "0";
    ListPtr<IString> addresses6List = config.getPropertyValue("addresses6");
    std::string addresses6String = "";
    for (const auto& addr : addresses6List)
        addresses6String += addr.toStdString() + ";";
    props["addresses6"] = addresses6String;
    StringPtr gateway6 = config.getPropertyValue("gateway6");
    props["gateway6"] = gateway6.toStdString();
}

ListPtr<IString> IpModificationUtils::populateAddresses(const std::string& addressesString)
{
    auto addresses = List<IString>();

    if (addressesString != "")
    {
        std::string address;
        std::stringstream ss(addressesString);
        while (std::getline(ss, address, ';'))
            if (!address.empty())
                addresses.pushBack(address);
    }

    return addresses;
}

PropertyObjectPtr IpModificationUtils::populateIpConfigProperties(const TxtProperties& txtProps)
{
    std::vector<std::string> txtKeys{"dhcp4", "addresses4", "gateway4", "dhcp6", "addresses6", "gateway6"};
    for (const auto& key : txtKeys)
    {
        if (const auto it = txtProps.find(key); it == txtProps.end())
            throw InvalidParameterException("Incomplete IP configuration");
    }

    auto config = PropertyObject();

    config.addProperty(BoolProperty("dhcp4", txtProps.at("dhcp4") == "1"));
    config.addProperty(ListProperty("addresses4", populateAddresses(txtProps.at("addresses4"))));
    config.addProperty(StringProperty("gateway4", txtProps.at("gateway4")));
    config.addProperty(BoolProperty("dhcp6", txtProps.at("dhcp6") == "1"));
    config.addProperty(ListProperty("addresses6", populateAddresses(txtProps.at("addresses6"))));
    config.addProperty(StringProperty("gateway6", txtProps.at("gateway6")));

    return config;
}

TxtProperties DiscoveryUtils::readTxtRecord(size_t size, const void* buffer, size_t rdata_offset, size_t rdata_length)
{
    mdns_record_txt_t txtbuffer[128];
    TxtProperties txtProperties;
    size_t parsed =
        mdns_record_parse_txt(buffer, size, rdata_offset, rdata_length, txtbuffer, sizeof(txtbuffer) / sizeof(mdns_record_txt_t));
    for (size_t itxt = 0; itxt < parsed; ++itxt)
    {
        std::string key(txtbuffer[itxt].key.str, txtbuffer[itxt].key.length);
        if (txtbuffer[itxt].value.length)
        {
            std::string value(txtbuffer[itxt].value.str, txtbuffer[itxt].value.length);
            txtProperties[key] = value;
        }
        else
        {
            txtProperties[key] = "";
        }
    }

    return txtProperties;
}

END_NAMESPACE_DISCOVERY_COMMON
