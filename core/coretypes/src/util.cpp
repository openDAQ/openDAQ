#include <coretypes/common.h>
#include <coretypes/errors.h>
#include <fmt/format.h>

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
