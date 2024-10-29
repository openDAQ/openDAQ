#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <ref_fb_module/classifier_fb_impl.h>
#include <ref_fb_module/power_fb_impl.h>
#include <ref_fb_module/ref_fb_module_impl.h>
#ifdef OPENDAQ_ENABLE_RENDERER
#include <ref_fb_module/renderer_fb_impl.h>
#endif
#include <ref_fb_module/scaling_fb_impl.h>
#include <ref_fb_module/statistics_fb_impl.h>
#include <ref_fb_module/trigger_fb_impl.h>
#include <ref_fb_module/fft_fb_impl.h>
#include <ref_fb_module/version.h>
#include <ref_fb_module/power_reader_fb_impl.h>
#include <ref_fb_module/struct_decoder_fb_impl.h>
#include <opendaq/module_info_factory.h>


BEGIN_NAMESPACE_REF_FB_MODULE
RefFBModule::RefFBModule(const ContextPtr& ctx)
    : Module(daq::ModuleInfo(
                 daq::VersionInfo(REF_FB_MODULE_MAJOR_VERSION, REF_FB_MODULE_MINOR_VERSION, REF_FB_MODULE_PATCH_VERSION),
                             "ReferenceFunctionBlockModule",
                             "ReferenceFunctionBlockModule"),
             ctx)
{
}

DictPtr<IString, IFunctionBlockType> RefFBModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

#ifdef OPENDAQ_ENABLE_RENDERER
    const auto typeRenderer = Renderer::RendererFbImpl::CreateType();
    types.set(typeRenderer.getId(), typeRenderer);
#endif

    const auto typeStatistics = Statistics::StatisticsFbImpl::CreateType();
    types.set(typeStatistics.getId(), typeStatistics);

    const auto typePower = Power::PowerFbImpl::CreateType();
    types.set(typePower.getId(), typePower);

    const auto typeScaling = Scaling::ScalingFbImpl::CreateType();
    types.set(typeScaling.getId(), typeScaling);

    const auto typeClassifier = Classifier::ClassifierFbImpl::CreateType();
    types.set(typeClassifier.getId(), typeClassifier);

    const auto typeTrigger = Trigger::TriggerFbImpl::CreateType();
    types.set(typeTrigger.getId(), typeTrigger);

    const auto typeFFT = FFT::FFTFbImpl::CreateType();
    types.set(typeFFT.getId(), typeFFT);

    const auto typePowerReader = PowerReader::PowerReaderFbImpl::CreateType();
    types.set(typePowerReader.getId(), typePowerReader);

    const auto typeStructDecoder = StructDecoder::StructDecoderFbImpl::CreateType();
    types.set(typeStructDecoder.getId(), typeStructDecoder);

    return types;
}

FunctionBlockPtr RefFBModule::onCreateFunctionBlock(const StringPtr& id,
                                                    const ComponentPtr& parent,
                                                    const StringPtr& localId,
                                                    const PropertyObjectPtr& config)
{
#ifdef OPENDAQ_ENABLE_RENDERER
    if (id == Renderer::RendererFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Renderer::RendererFbImpl>(context, parent, localId);
        return fb;
    }
#endif
    if (id == Statistics::StatisticsFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Statistics::StatisticsFbImpl>(context, parent, localId, config);
        return fb;
    }
    if (id == Power::PowerFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Power::PowerFbImpl>(context, parent, localId);
        return fb;
    }
    if (id == Scaling::ScalingFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Scaling::ScalingFbImpl>(context, parent, localId);
        return fb;
    }
    if (id == Classifier::ClassifierFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Classifier::ClassifierFbImpl>(context, parent, localId);
        return fb;
    }
    if (id == Trigger::TriggerFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Trigger::TriggerFbImpl>(context, parent, localId, config);
        return fb;
    }
    if (id == FFT::FFTFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, FFT::FFTFbImpl>(context, parent, localId);
        return fb;
    }
    if (id == PowerReader::PowerReaderFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, PowerReader::PowerReaderFbImpl>(context, parent, localId);
        return fb;
    }
    if (id == StructDecoder::StructDecoderFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, StructDecoder::StructDecoderFbImpl>(context, parent, localId);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_REF_FB_MODULE
