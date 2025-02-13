#include <utility>

#include <coretypes/version_info_factory.h>
#include <opendaq/module_impl.h>
#include <opendaq/opendaq.h>

#include <basic_recorder_module/basic_recorder_impl.h>
#include <basic_recorder_module/basic_recorder_module_impl.h>
#include <basic_recorder_module/common.h>
#include <basic_recorder_module/version.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE

BasicRecorderModule::BasicRecorderModule(ContextPtr context)
    : Module(
        MODULE_NAME,
        VersionInfo(
            BASIC_RECORDER_MODULE_MAJOR_VERSION,
            BASIC_RECORDER_MODULE_MINOR_VERSION,
            BASIC_RECORDER_MODULE_PATCH_VERSION),
        std::move(context),
        MODULE_NAME)
    , basicRecorderType(BasicRecorderImpl::createType())
{
}

DictPtr<IString, IFunctionBlockType>
BasicRecorderModule::onGetAvailableFunctionBlockTypes()
{
    auto id = basicRecorderType.getId();
    return Dict<IString, IFunctionBlockType>(
    {
        { basicRecorderType.getId(), basicRecorderType },
    });
}

FunctionBlockPtr BasicRecorderModule::onCreateFunctionBlock(
    const StringPtr& id,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const PropertyObjectPtr& config)
{
    if (id == basicRecorderType.getId())
        return createWithImplementation<IFunctionBlock, BasicRecorderImpl>(context, parent, localId, config);

    throw NotFoundException("This module does not support function block type '" + id + "'");
}

END_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE
