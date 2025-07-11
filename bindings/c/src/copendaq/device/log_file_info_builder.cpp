//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:07:09.
// </auto-generated>
//------------------------------------------------------------------------------

#include <copendaq/device/log_file_info_builder.h>

#include <opendaq/opendaq.h>

#include <copendaq_private.h>

const daqIntfID DAQ_LOG_FILE_INFO_BUILDER_INTF_ID = { daq::ILogFileInfoBuilder::Id.Data1, daq::ILogFileInfoBuilder::Id.Data2, daq::ILogFileInfoBuilder::Id.Data3, daq::ILogFileInfoBuilder::Id.Data4_UInt64 };

daqErrCode daqLogFileInfoBuilder_build(daqLogFileInfoBuilder* self, daqLogFileInfo** logFileInfo)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->build(reinterpret_cast<daq::ILogFileInfo**>(logFileInfo));
}

daqErrCode daqLogFileInfoBuilder_getLocalPath(daqLogFileInfoBuilder* self, daqString** localPath)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->getLocalPath(reinterpret_cast<daq::IString**>(localPath));
}

daqErrCode daqLogFileInfoBuilder_setLocalPath(daqLogFileInfoBuilder* self, daqString* localPath)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->setLocalPath(reinterpret_cast<daq::IString*>(localPath));
}

daqErrCode daqLogFileInfoBuilder_getName(daqLogFileInfoBuilder* self, daqString** name)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->getName(reinterpret_cast<daq::IString**>(name));
}

daqErrCode daqLogFileInfoBuilder_setName(daqLogFileInfoBuilder* self, daqString* name)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->setName(reinterpret_cast<daq::IString*>(name));
}

daqErrCode daqLogFileInfoBuilder_getId(daqLogFileInfoBuilder* self, daqString** id)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->getId(reinterpret_cast<daq::IString**>(id));
}

daqErrCode daqLogFileInfoBuilder_setId(daqLogFileInfoBuilder* self, daqString* id)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->setId(reinterpret_cast<daq::IString*>(id));
}

daqErrCode daqLogFileInfoBuilder_getDescription(daqLogFileInfoBuilder* self, daqString** description)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->getDescription(reinterpret_cast<daq::IString**>(description));
}

daqErrCode daqLogFileInfoBuilder_setDescription(daqLogFileInfoBuilder* self, daqString* description)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->setDescription(reinterpret_cast<daq::IString*>(description));
}

daqErrCode daqLogFileInfoBuilder_getSize(daqLogFileInfoBuilder* self, daqSizeT* size)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->getSize(size);
}

daqErrCode daqLogFileInfoBuilder_setSize(daqLogFileInfoBuilder* self, daqSizeT size)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->setSize(size);
}

daqErrCode daqLogFileInfoBuilder_getEncoding(daqLogFileInfoBuilder* self, daqString** encoding)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->getEncoding(reinterpret_cast<daq::IString**>(encoding));
}

daqErrCode daqLogFileInfoBuilder_setEncoding(daqLogFileInfoBuilder* self, daqString* encoding)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->setEncoding(reinterpret_cast<daq::IString*>(encoding));
}

daqErrCode daqLogFileInfoBuilder_getLastModified(daqLogFileInfoBuilder* self, daqString** lastModified)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->getLastModified(reinterpret_cast<daq::IString**>(lastModified));
}

daqErrCode daqLogFileInfoBuilder_setLastModified(daqLogFileInfoBuilder* self, daqString* lastModified)
{
    return reinterpret_cast<daq::ILogFileInfoBuilder*>(self)->setLastModified(reinterpret_cast<daq::IString*>(lastModified));
}

daqErrCode daqLogFileInfoBuilder_createLogFileInfoBuilder(daqLogFileInfoBuilder** obj)
{
    daq::ILogFileInfoBuilder* ptr = nullptr;
    daqErrCode err = daq::createLogFileInfoBuilder(&ptr);
    *obj = reinterpret_cast<daqLogFileInfoBuilder*>(ptr);
    return err;
}
