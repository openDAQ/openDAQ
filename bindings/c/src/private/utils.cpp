#include "private/utils.h"

#include <opendaq/opendaq.h>

namespace copendaq::utils
{

daq::IntfID toDaqIntfId(IntfID id)
{
    return daq::IntfID{id.Data1, id.Data2, id.Data3, {.Data4_UInt64 = id.Data4}};
}

}  // namespace copendaq::utils