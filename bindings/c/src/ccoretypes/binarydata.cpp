//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 01.04.2025 17:01:44.
// </auto-generated>
//------------------------------------------------------------------------------

#include "ccoretypes/binarydata.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID BINARY_DATA_INTF_ID = { daq::IBinaryData::Id.Data1, daq::IBinaryData::Id.Data2, daq::IBinaryData::Id.Data3, daq::IBinaryData::Id.Data4_UInt64 };

ErrCode BinaryData_getAddress(BinaryData* self, void** data)
{
    return reinterpret_cast<daq::IBinaryData*>(self)->getAddress(data);
}

ErrCode BinaryData_getSize(BinaryData* self, SizeT* size)
{
    return reinterpret_cast<daq::IBinaryData*>(self)->getSize(size);
}

ErrCode BinaryData_createBinaryData(BinaryData** obj, SizeT size)
{
    daq::IBinaryData* ptr = nullptr;
    ErrCode err = daq::createBinaryData(&ptr, size);
    *obj = reinterpret_cast<BinaryData*>(ptr);
    return err;
}
