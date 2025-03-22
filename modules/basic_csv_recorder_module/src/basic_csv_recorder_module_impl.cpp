#include <utility>

#include <coretypes/version_info_factory.h>
#include <opendaq/module_impl.h>
#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/basic_csv_recorder_impl.h>
#include <basic_csv_recorder_module/basic_csv_recorder_module_impl.h>
#include <basic_csv_recorder_module/common.h>
#include <basic_csv_recorder_module/version.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

BasicCsvRecorderModule::BasicCsvRecorderModule(ContextPtr context)
    : Module(
        MODULE_NAME,
        VersionInfo(
            BASIC_CSV_RECORDER_MODULE_MAJOR_VERSION,
            BASIC_CSV_RECORDER_MODULE_MINOR_VERSION,
            BASIC_CSV_RECORDER_MODULE_PATCH_VERSION),
        std::move(context),
        MODULE_NAME)
    , basicCsvRecorderType(BasicCsvRecorderImpl::createType())
{
}

DictPtr<IString, IFunctionBlockType>
BasicCsvRecorderModule::onGetAvailableFunctionBlockTypes()
{
    auto id = basicCsvRecorderType.getId();
    return Dict<IString, IFunctionBlockType>(
    {
        { basicCsvRecorderType.getId(), basicCsvRecorderType },
    });
}

FunctionBlockPtr BasicCsvRecorderModule::onCreateFunctionBlock(
    const StringPtr& id,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const PropertyObjectPtr& config)
{
    if (id == basicCsvRecorderType.getId())
        return createWithImplementation<IFunctionBlock, BasicCsvRecorderImpl>(context, parent, localId, config);

    throw NotFoundException("This module does not support function block type '" + id + "'");
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
