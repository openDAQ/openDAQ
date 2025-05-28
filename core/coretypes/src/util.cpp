#include <coretypes/common.h>
#include <coretypes/errors.h>
#include <fmt/format.h>
#include <regex>

#include "coretypes/ctutils.h"

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

namespace detail
{

uint32_t hexStringToUint32(const char* hexStr, size_t length)
{
    if (hexStr == nullptr || length == 0)
    {
        throw std::invalid_argument("Empty or null input string");
    }

    uint32_t result = 0;

    for (size_t i = 0; i < length; ++i)
    {
        char c = hexStr[i];
        uint32_t digit;

        if (c >= '0' && c <= '9')
        {
            digit = c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            digit = c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            digit = c - 'A' + 10;
        }
        else
        {
            throw std::invalid_argument("Invalid hexadecimal character");
        }

        if (result > (std::numeric_limits<uint32_t>::max() - digit) / 16)
        {
            throw std::overflow_error("uint32_t overflow");
        }

        result = result * 16 + digit;
    }

    return result;
}

}

extern "C"
daq::ErrCode PUBLIC_EXPORT daqStringToInterfaceId(daq::ConstCharPtr guidStr, daq::IntfID& iid)
{
    try
    {
        const auto len = std::strlen(guidStr);
        if (len != 38)
            return OPENDAQ_ERR_INVALIDPARAMETER;

        if (guidStr[0] != '{' || guidStr[len - 1] != '}')
            return OPENDAQ_ERR_INVALIDPARAMETER;

        iid.Data1 = detail::hexStringToUint32(guidStr + 1, 8);
        if (guidStr[9] != '-')
            return OPENDAQ_ERR_INVALIDPARAMETER;
        iid.Data2 = static_cast<uint16_t>(detail::hexStringToUint32(guidStr + 10, 4));
        if (guidStr[14] != '-')
            return OPENDAQ_ERR_INVALIDPARAMETER;
        iid.Data3 = static_cast<uint16_t>(detail::hexStringToUint32(guidStr + 15, 4));
        if (guidStr[19] != '-')
            return OPENDAQ_ERR_INVALIDPARAMETER;

        iid.Data4[0] = static_cast<uint8_t>(detail::hexStringToUint32(guidStr + 20, 2));
        iid.Data4[1] = static_cast<uint8_t>(detail::hexStringToUint32(guidStr + 22, 2));
        if (guidStr[24] != '-')
            return OPENDAQ_ERR_INVALIDPARAMETER;
        iid.Data4[2] = static_cast<uint8_t>(detail::hexStringToUint32(guidStr + 25, 2));
        iid.Data4[3] = static_cast<uint8_t>(detail::hexStringToUint32(guidStr + 27, 2));
        iid.Data4[4] = static_cast<uint8_t>(detail::hexStringToUint32(guidStr + 29, 2));
        iid.Data4[5] = static_cast<uint8_t>(detail::hexStringToUint32(guidStr + 31, 2));
        iid.Data4[6] = static_cast<uint8_t>(detail::hexStringToUint32(guidStr + 33, 2));
        iid.Data4[7] = static_cast<uint8_t>(detail::hexStringToUint32(guidStr + 35, 2));
    }
    catch (...)
    {
        return OPENDAQ_ERR_INVALIDPARAMETER;  // Conversion error
    }

    return OPENDAQ_SUCCESS;
}
