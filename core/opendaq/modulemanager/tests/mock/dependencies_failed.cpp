#include <opendaq/module.h>
#include <coretypes/common.h>
#include <coretypes/stringobject.h>
#include <opendaq/version.h>

extern "C"
daq::ErrCode PUBLIC_EXPORT createModule(daq::IModule**)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR);
}

static void enumerateMetadataFieldWrapper(const char* key, const char* value, void* userData)
{
    auto dictRawPtr = static_cast<daq::IDict*>(userData);
    daq::DictPtr<daq::IString, daq::IString> dictPtr = daq::DictPtr<daq::IString, daq::IString>::Borrow(dictRawPtr);
    dictPtr[key] = value;
}

extern "C"
daq::ErrCode PUBLIC_EXPORT checkDependencies()
{
    auto coreVersionMetadata = daq::Dict<daq::IString, daq::IString>();

    daq::ErrCode errCode = getSdkCoreVersionMetadata(&enumerateMetadataFieldWrapper, coreVersionMetadata.getObject());
    OPENDAQ_RETURN_IF_FAILED(errCode, "Getting SDK version metadata failed");

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "Mock module checkDependencies failure");
}
