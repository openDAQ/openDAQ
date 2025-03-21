#include <discovery_common/daq_discovery_common.h>
#include <cctype>

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

END_NAMESPACE_DISCOVERY_COMMON
