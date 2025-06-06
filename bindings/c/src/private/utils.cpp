#include <private/utils.h>

namespace copendaq::utils
{

daq::IntfID toDaqIntfId(daqIntfID id)
{
    auto intf = daq::IntfID{ id.Data1, id.Data2, id.Data3};
    intf.Data4_UInt64 = id.Data4; // could be assigned on construction with c++20
    return intf;
}

daq::PropertyEventType toDaqPropertyEventType(daqPropertyEventType type)
{
    switch (type)
    {
        case daqPropertyEventType::daqPropertyEventTypeEventTypeClear:
            return daq::PropertyEventType::Clear;
        case daqPropertyEventType::daqPropertyEventTypeEventTypeRead:
            return daq::PropertyEventType::Read;
        case daqPropertyEventType::daqPropertyEventTypeEventTypeUpdate:
        default:
            return daq::PropertyEventType::Update; // Default case to avoid compiler warnings
    }
}

daqPropertyEventType toCPropertyEventType(daq::PropertyEventType type)
{
    switch (type)
    {
        case daq::PropertyEventType::Clear:
            return daqPropertyEventType::daqPropertyEventTypeEventTypeClear;
        case daq::PropertyEventType::Read:
            return daqPropertyEventType::daqPropertyEventTypeEventTypeRead;
        case daq::PropertyEventType::Update:
        default:
            return daqPropertyEventType::daqPropertyEventTypeEventTypeUpdate; // Default case to avoid compiler warnings
    }
}

}  // namespace copendaq::utils