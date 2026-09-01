#include <utility>

#include <coretypes/version_info_factory.h>
#include <opendaq/module_impl.h>
#include <opendaq/opendaq.h>

#include <file_recorder_module/common.h>
#include <file_recorder_module/file_player_fb_impl.h>
#include <file_recorder_module/file_recorder_fb_impl.h>
#include <file_recorder_module/file_recorder_module_impl.h>
#include <file_recorder_module/version.h>

BEGIN_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE

FileRecorderModule::FileRecorderModule(ContextPtr context)
    : Module(MODULE_NAME,
             VersionInfo(FILE_RECORDER_MODULE_MAJOR_VERSION, FILE_RECORDER_MODULE_MINOR_VERSION, FILE_RECORDER_MODULE_PATCH_VERSION),
             std::move(context),
             MODULE_NAME)
{
}

DictPtr<IString, IFunctionBlockType> FileRecorderModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    const auto recorderType = FileRecorderFbImpl::createType();
    types.set(recorderType.getId(), recorderType);

    const auto playerType = FilePlayerFbImpl::createType();
    types.set(playerType.getId(), playerType);

    return types;
}

FunctionBlockPtr FileRecorderModule::onCreateFunctionBlock(const StringPtr& id,
                                                           const ComponentPtr& parent,
                                                           const StringPtr& localId,
                                                           const PropertyObjectPtr& config)
{
    if (id == FileRecorderFbImpl::createType().getId())
        return createWithImplementation<IFunctionBlock, FileRecorderFbImpl>(context, parent, localId, config);

    if (id == FilePlayerFbImpl::createType().getId())
        return createWithImplementation<IFunctionBlock, FilePlayerFbImpl>(context, parent, localId, config);

    DAQ_THROW_EXCEPTION(NotFoundException, "This module does not support function block type '" + id + "'");
}

END_NAMESPACE_OPENDAQ_FILE_RECORDER_MODULE
