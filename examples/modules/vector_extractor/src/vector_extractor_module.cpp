#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <asam_cmp_decoder_module/refFB64ByteVectorSignalDecoder.h>
#include <asam_cmp_decoder_module/asam_cmp_decoder_module.h>
#include <asam_cmp_decoder_module/version.h>
#include <iostream>

BEGIN_NAMESPACE_ASAM_CMP_DECODER_MODULE

AsamCmpDecoderModule::AsamCmpDecoderModule(ContextPtr context)
    : Module("AsamCmpDecoderModule",
             VersionInfo(ASAM_CMP_DECODER_MODULE_MAJOR_VERSION, ASAM_CMP_DECODER_MODULE_MINOR_VERSION, ASAM_CMP_DECODER_MODULE_PATCH_VERSION),
             std::move(context),
             "AsamCmpDecoderModule")
{
}

DictPtr<IString, IFunctionBlockType> AsamCmpDecoderModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

    const auto ref64 = RefFB64ByteVectorSignalDecoderImpl::CreateType();
    types.set(ref64.getId(), ref64);

    return types;
}

FunctionBlockPtr AsamCmpDecoderModule::onCreateFunctionBlock(const StringPtr& id,
                                                      const ComponentPtr& parent,
                                                      const StringPtr& localId,
                                                      const PropertyObjectPtr& config)
{
    if (id == RefFB64ByteVectorSignalDecoderImpl::CreateType().getId())
    {
        FunctionBlockPtr fb =
            createWithImplementation<IFunctionBlock, RefFB64ByteVectorSignalDecoderImpl>(
                context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_ASAM_CMP_DECODER_MODULE
