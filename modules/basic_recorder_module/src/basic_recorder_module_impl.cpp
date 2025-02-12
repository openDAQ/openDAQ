#include <utility>

#include <coretypes/version_info_factory.h>
#include <opendaq/module_impl.h>
#include <opendaq/opendaq.h>

#include <basic_recorder_module/basic_recorder_impl.h>
#include <basic_recorder_module/basic_recorder_module_impl.h>
#include <basic_recorder_module/common.h>
#include <basic_recorder_module/version.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE

BasicRecorderModule::BasicRecorderModule(daq::ContextPtr context)
    : daq::Module(
        MODULE_NAME,
        daq::VersionInfo(
            BASIC_RECORDER_MODULE_MAJOR_VERSION,
            BASIC_RECORDER_MODULE_MINOR_VERSION,
            BASIC_RECORDER_MODULE_PATCH_VERSION),
        std::move(context),
        MODULE_NAME)
    , basicRecorderType(BasicRecorderImpl::createType())
{
}

daq::DictPtr<daq::IString, daq::IFunctionBlockType>
BasicRecorderModule::onGetAvailableFunctionBlockTypes()
{
    auto id = basicRecorderType.getId();
    return daq::Dict<daq::IString, daq::IFunctionBlockType>(
    {
        { basicRecorderType.getId(), basicRecorderType },
    });
}

daq::FunctionBlockPtr BasicRecorderModule::onCreateFunctionBlock(
    const daq::StringPtr& id,
    const daq::ComponentPtr& parent,
    const daq::StringPtr& localId,
    const daq::PropertyObjectPtr& config)
{
    if (id == basicRecorderType.getId())
        return daq::createWithImplementation<daq::IFunctionBlock, BasicRecorderImpl>(context, parent, localId, config);

    throw daq::NotFoundException("This module does not support function block type '" + id + "'");
}

END_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE
