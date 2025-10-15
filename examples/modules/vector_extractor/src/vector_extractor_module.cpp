#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <vector_extractor/vector_extractor.h>
#include <vector_extractor/vector_extractor_module.h>
#include <vector_extractor/version.h>
#include <iostream>

BEGIN_NAMESPACE_VECTOR_EXTRACTOR_MODULE

VectorExtractorModule::VectorExtractorModule(ContextPtr context)
    : Module("VectorExtractorModule",
             VersionInfo(VECTOR_EXTRACTOR_MODULE_MAJOR_VERSION, VECTOR_EXTRACTOR_MODULE_MINOR_VERSION, VECTOR_EXTRACTOR_MODULE_PATCH_VERSION),
             std::move(context),
             "VectorExtractorModule")
{
}

DictPtr<IString, IFunctionBlockType> VectorExtractorModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    const auto ref64 = VectorExtractorImpl::CreateType();
    types.set(ref64.getId(), ref64);

    return types;
}

FunctionBlockPtr VectorExtractorModule::onCreateFunctionBlock(const StringPtr& id,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const PropertyObjectPtr& config)
{
    if (id == VectorExtractorImpl::CreateType().getId())
    {
        FunctionBlockPtr fb =
            createWithImplementation<IFunctionBlock, VectorExtractorImpl>(
                context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_VECTOR_EXTRACTOR_MODULE
