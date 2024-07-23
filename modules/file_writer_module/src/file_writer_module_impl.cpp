#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <file_writer_module/file_writer_module_impl.h>
#include <file_writer_module/parquet_fb_impl.h>
#include <file_writer_module/version.h>

BEGIN_NAMESPACE_FILE_WRITER_MODULE

FileWriterModule::FileWriterModule(ContextPtr ctx)
    : Module("File writer module",
             daq::VersionInfo(FILE_WRITER_MODULE_MAJOR_VERSION, FILE_WRITER_MODULE_MINOR_VERSION, FILE_WRITER_MODULE_PATCH_VERSION),
             std::move(ctx),
             "FileWriter")
{
}

DictPtr<IString, IFunctionBlockType> FileWriterModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    auto parquet = FileWriter::ParquetFbImpl::CreateType();
    types.set(parquet.getId(), parquet);

    return types;
}

FunctionBlockPtr FileWriterModule::onCreateFunctionBlock(const StringPtr& id,
                                                    const ComponentPtr& parent,
                                                    const StringPtr& localId,
                                                    const PropertyObjectPtr& config)
{
    if (id == FileWriter::ParquetFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, FileWriter::ParquetFbImpl>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_FILE_WRITER_MODULE

