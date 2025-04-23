#include <discovery_common/daq_discovery_common.h>
#include <cctype>
#include <map>
#include <set>
#include <coreobjects/property_object_protected_ptr.h>
#include <coreobjects/property_object_internal_ptr.h>

BEGIN_NAMESPACE_DISCOVERY_COMMON

void IpModificationUtils::encodeIpConfiguration(const PropertyObjectPtr& config, TxtProperties& props)
{
    const bool dhcp4Mode = config.getPropertyValue("dhcp4");
    props["dhcp4"] = dhcp4Mode ? "1" : "0";
    StringPtr address4 = config.getPropertyValue("address4");
    props["address4"] = address4.toStdString();
    StringPtr gateway4 = config.getPropertyValue("gateway4");
    props["gateway4"] = gateway4.toStdString();

    const bool dhcp6Mode = config.getPropertyValue("dhcp6");
    props["dhcp6"] = dhcp6Mode ? "1" : "0";
    StringPtr address6 = config.getPropertyValue("address6");
    props["address6"] = address6.toStdString();
    StringPtr gateway6 = config.getPropertyValue("gateway6");
    props["gateway6"] = gateway6.toStdString();
}

PropertyObjectPtr IpModificationUtils::populateIpConfigProperties(const TxtProperties& txtProps)
{
    std::vector<std::string> txtKeys{"dhcp4", "address4", "gateway4", "dhcp6", "address6", "gateway6"};
    for (const auto& key : txtKeys)
    {
        if (const auto it = txtProps.find(key); it == txtProps.end())
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Incomplete IP configuration");
    }

    auto config = PropertyObject();

    config.addProperty(BoolProperty("dhcp4", txtProps.at("dhcp4") == "1"));
    config.addProperty(StringProperty("address4", txtProps.at("address4")));
    config.addProperty(StringProperty("gateway4", txtProps.at("gateway4")));
    config.addProperty(BoolProperty("dhcp6", txtProps.at("dhcp6") == "1"));
    config.addProperty(StringProperty("address6", txtProps.at("address6")));
    config.addProperty(StringProperty("gateway6", txtProps.at("gateway6")));

    return config;
}

size_t DiscoveryUtils::getTxtRecordsCount(const void* buffer, size_t size, size_t offset, size_t length)
{
    // the calculation logic is from mdns_record_parse_txt
    size_t count = 0;
    const char* strdata;
    size_t end = offset + length;

    if (size < end)
        end = size;

    while (offset < end)
    {
        strdata = (const char*)MDNS_POINTER_OFFSET(buffer, offset);
        size_t sublength = *(const unsigned char*)strdata;

        if (sublength >= (end - offset))
            break;

        offset += sublength + 1;
        count++;
    }

    return count;
}

TxtProperties DiscoveryUtils::readTxtRecord(size_t size, const void* buffer, size_t rdata_offset, size_t rdata_length)
{
    size_t recordsCount = getTxtRecordsCount(buffer, size, rdata_offset, rdata_length);
    std::vector<mdns_record_txt_t> txtbuffer(recordsCount);

    TxtProperties txtProperties;
    size_t parsed =
        mdns_record_parse_txt(buffer, size, rdata_offset, rdata_length, txtbuffer.data(), recordsCount);

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

std::string DiscoveryUtils::extractRecordName(const void* buffer, size_t nameOffset, size_t bufferSize)
{
    std::vector<char> nameBuffer(256);
    mdns_string_t nameTmp = mdns_string_extract(buffer, bufferSize, &nameOffset, nameBuffer.data(), nameBuffer.size());
    return std::string(MDNS_STRING_ARGS(nameTmp));
}

std::string DiscoveryUtils::toTxtValue(const char* source, size_t length)
{
    std::string result;
    result.reserve(length);

    for (const char* ptr = source; *ptr && result.size() < length; ++ptr)
    {
        // Replace whitespace, '=' or invalid characters with a space
        if (isspace(static_cast<unsigned char>(*ptr)) || *ptr == '=' || !isprint(static_cast<unsigned char>(*ptr)))
            result += ' ';
        else
            result += *ptr;
    }

    return result;
}

TxtProperties DiscoveryUtils::connectedClientsInfoToTxt(const PropertyObjectPtr& connectedClientsInfo)
{
    TxtProperties result;

    const auto zeroPadNumber =
        [width = std::to_string(connectedClientsInfo.getAllProperties().getCount() - 1).size()](size_t index)
        {
            return std::string(width - std::to_string(index).size(), '0') + std::to_string(index);
        };

    size_t index = 0;
    for (const auto& clientInfoProperty : connectedClientsInfo.getAllProperties())
    {
        const PropertyObjectPtr connectedClientPropObject = clientInfoProperty.getValue();

        // replace client ID string with lexicographically ordered index string
        std::string keyPrefix = std::string(CONNECTED_CLIENT_INFO_KEY_PREFIX) + zeroPadNumber(index) + "--";
        ++index;

        for (const auto& property : connectedClientPropObject.getAllProperties())
        {
            if (property.getValueType() == CoreType::ctString)
            {
                std::string propName = property.getName().toStdString();
                bool invalidTxtKey = std::any_of(propName.begin(), propName.end(),
                                                 [](unsigned char c) { return c == '=' || !std::isprint(c); });
                if (invalidTxtKey)
                    continue;
                std::string key = keyPrefix + propName;
                if (key.size() < 254) // insert if at least first symbol of value fits into TXT record
                {
                    std::string propValue = property.getValue().asPtr<IString>().toStdString();
                    std::string txtValue = toTxtValue(propValue.c_str(), 254 - key.size());
                    result.insert({key, txtValue});
                }
            }
        }
    }

    return result;
}

void DiscoveryUtils::populateConnectedClientsInfo(PropertyObjectPtr& deviceInfo,
                                                  const PropertyObjectPtr& defaultClientInfo,
                                                  const TxtProperties& txtKeyValuePairs)
{
    if (!defaultClientInfo.assigned() || !deviceInfo.assigned())
        return;

    const auto setProtectedPropertyValue = [](PropertyObjectPtr propertyObject,
                                              const StringPtr& propertyName,
                                              const BaseObjectPtr& propertyValue)
    {
        if (auto protectedObj = propertyObject.asPtrOrNull<IPropertyObjectProtected>(); protectedObj.assigned())
            protectedObj.setProtectedPropertyValue(propertyName, propertyValue);
        else
            propertyObject->setPropertyValue(propertyName, propertyValue); // Ignore errors
    };

    std::set<std::string> orderedClientIds;
    PropertyObjectPtr clientsInfo = deviceInfo.getPropertyValue("activeClientConnections");
    for (const auto& [txtKey, txtValue] : txtKeyValuePairs)
    {
        std::string prefix(CONNECTED_CLIENT_INFO_KEY_PREFIX);
        if (txtKey.find(prefix) == std::string::npos)
            continue;

        auto prefixSize = prefix.size();
        if (const auto pos = txtKey.find("--", prefixSize); pos != std::string::npos && pos < (txtKey.size() - 2))
        {
            std::string clientId = txtKey.substr(prefixSize, pos - prefixSize);
            std::string propName = txtKey.substr(pos + 2);

            if (!clientsInfo.hasProperty(clientId))
            {
                orderedClientIds.insert(clientId);
                clientsInfo.addProperty(ObjectProperty(clientId, defaultClientInfo.asPtr<IPropertyObjectInternal>().clone()));
            }

            PropertyObjectPtr clientInfo = clientsInfo.getPropertyValue(clientId);
            if (clientInfo.hasProperty(propName))
                setProtectedPropertyValue(clientInfo, String(propName), static_cast<daq::BaseObjectPtr>(txtValue));
            else
                clientInfo.addProperty(StringPropertyBuilder(propName, txtValue).setReadOnly(true).build());

            setProtectedPropertyValue(clientsInfo, String(clientId), clientInfo);
        }
    }

    // restore lexicographical order to match the original server order
    auto propertyOrder = List<IString>();
    for (const auto& clientId : orderedClientIds)
        propertyOrder.pushBack(String(clientId));
    clientsInfo.setPropertyOrder(propertyOrder);

    setProtectedPropertyValue(deviceInfo, "activeClientConnections", clientsInfo);
}

END_NAMESPACE_DISCOVERY_COMMON
