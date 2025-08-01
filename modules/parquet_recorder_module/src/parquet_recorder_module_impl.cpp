#include <utility>

#include <coretypes/version_info_factory.h>
#include <opendaq/module_impl.h>
#include <opendaq/opendaq.h>

#include <parquet_recorder_module/common.h>
#include <parquet_recorder_module/parquet_recorder_impl.h>
#include <parquet_recorder_module/parquet_recorder_module_impl.h>
#include <parquet_recorder_module/version.h>

BEGIN_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE

ParquetRecorderModule::ParquetRecorderModule(ContextPtr context)
    : Module(
          MODULE_NAME,
          VersionInfo(PARQUET_RECORDER_MODULE_MAJOR_VERSION, PARQUET_RECORDER_MODULE_MINOR_VERSION, PARQUET_RECORDER_MODULE_PATCH_VERSION),
          std::move(context),
          MODULE_NAME)
    , parquetRecorderType(ParquetRecorderImpl::createType())
{
}

DictPtr<IString, IFunctionBlockType> ParquetRecorderModule::onGetAvailableFunctionBlockTypes()
{
    auto id = parquetRecorderType.getId();
    return Dict<IString, IFunctionBlockType>({
        {parquetRecorderType.getId(), parquetRecorderType},
    });
}

FunctionBlockPtr ParquetRecorderModule::onCreateFunctionBlock(const StringPtr& id,
                                                              const ComponentPtr& parent,
                                                              const StringPtr& localId,
                                                              const PropertyObjectPtr& config)
{
    if (id == parquetRecorderType.getId())
        return createWithImplementation<IFunctionBlock, ParquetRecorderImpl>(context, parent, localId, config);

    throw NotFoundException("This module does not support function block type '" + id + "'");
}

END_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE
