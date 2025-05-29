#include <private/utils.h>

namespace copendaq::utils
{

daq::IntfID toDaqIntfId(IntfID id)
{
    auto intf = daq::IntfID{ id.Data1, id.Data2, id.Data3};
    intf.Data4_UInt64 = id.Data4; // could be assigned on construction with c++20
    return intf;
}

daq::PropertyEventType toDaqPropertyEventType(PropertyEventType type)
{
    switch (type)
    {
        case PropertyEventType::PropertyEventTypeEventTypeClear:
            return daq::PropertyEventType::Clear;
        case PropertyEventType::PropertyEventTypeEventTypeRead:
            return daq::PropertyEventType::Read;
        case PropertyEventType::PropertyEventTypeEventTypeUpdate:
        default:
            return daq::PropertyEventType::Update; // Default case to avoid compiler warnings
    }
}

PropertyEventType toCPropertyEventType(daq::PropertyEventType type)
{
    switch (type)
    {
        case daq::PropertyEventType::Clear:
            return PropertyEventType::PropertyEventTypeEventTypeClear;
        case daq::PropertyEventType::Read:
            return PropertyEventType::PropertyEventTypeEventTypeRead;
        case daq::PropertyEventType::Update:
        default:
            return PropertyEventType::PropertyEventTypeEventTypeUpdate; // Default case to avoid compiler warnings
    }
}

}  // namespace copendaq::utils