//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:07:26.
// </auto-generated>
//------------------------------------------------------------------------------

#include <copendaq/reader/reader.h>

#include <opendaq/opendaq.h>

#include <copendaq_private.h>

const daqIntfID DAQ_READER_INTF_ID = { daq::IReader::Id.Data1, daq::IReader::Id.Data2, daq::IReader::Id.Data3, daq::IReader::Id.Data4_UInt64 };

daqErrCode daqReader_getAvailableCount(daqReader* self, daqSizeT* count)
{
    return reinterpret_cast<daq::IReader*>(self)->getAvailableCount(count);
}

daqErrCode daqReader_setOnDataAvailable(daqReader* self, daqProcedure* callback)
{
    return reinterpret_cast<daq::IReader*>(self)->setOnDataAvailable(reinterpret_cast<daq::IProcedure*>(callback));
}

daqErrCode daqReader_getEmpty(daqReader* self, daqBool* empty)
{
    return reinterpret_cast<daq::IReader*>(self)->getEmpty(empty);
}
