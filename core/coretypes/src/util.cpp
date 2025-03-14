#include <coretypes/common.h>
#include <coretypes/errors.h>
#include <fmt/format.h>

#include <regex>

extern "C"
daq::ErrCode PUBLIC_EXPORT daqInterfaceIdToString(const daq::IntfID& iid, daq::CharPtr dest)
{
    try
    {
        auto result = fmt::format_to_n(dest,
                                       38,
                                       "{{{:08X}-{:04X}-{:04X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:2X}{:02X}{:02X}}}",
                                       iid.Data1,
                                       iid.Data2,
                                       iid.Data3,
                                       iid.Data4[0],
                                       iid.Data4[1],
                                       iid.Data4[2],
                                       iid.Data4[3],
                                       iid.Data4[4],
                                       iid.Data4[5],
                                       iid.Data4[6],
                                       iid.Data4[7]);

        result.out[0] = '\0';
        return result.size == 38
            ? OPENDAQ_SUCCESS
            : OPENDAQ_ERR_GENERALERROR;
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }
}

extern "C"
daq::ErrCode PUBLIC_EXPORT stringToDaqInterfaceId(const std::string& guidStr, daq::IntfID& iid)
{
    std::regex guidRegex(
        R"(\{([\dA-Fa-f]{8})-([\dA-Fa-f]{4})-([\dA-Fa-f]{4})-([\dA-Fa-f]{2})([\dA-Fa-f]{2})-((?:[\dA-Fa-f]{2}){6})\})");
    
    std::smatch match;
    if (!std::regex_match(guidStr, match, guidRegex))
    {
        return OPENDAQ_ERR_GENERALERROR; // Invalid format
    }

    try
    {
        iid.Data1 = std::stoul(match[1].str(), nullptr, 16);
        iid.Data2 = static_cast<uint16_t>(std::stoul(match[2].str(), nullptr, 16));
        iid.Data3 = static_cast<uint16_t>(std::stoul(match[3].str(), nullptr, 16));

        // First two bytes of Data4
        iid.Data4[0] = static_cast<uint8_t>(std::stoul(match[4].str(), nullptr, 16));
        iid.Data4[1] = static_cast<uint8_t>(std::stoul(match[5].str(), nullptr, 16));

        // Last six bytes of Data4
        std::string lastPart = match[6].str();
        for (size_t i = 0; i < 6; ++i)
        {
            iid.Data4[2 + i] = static_cast<uint8_t>(std::stoul(lastPart.substr(i * 2, 2), nullptr, 16));
        }
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR; // Conversion error
    }

    return OPENDAQ_SUCCESS;
}
