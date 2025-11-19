#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <ref_fb_module/classifier_fb_impl.h>
#include <ref_fb_module/power_fb_impl.h>
#include <ref_fb_module/ref_fb_module_impl.h>
#ifdef OPENDAQ_ENABLE_RENDERER
#include <ref_fb_module/renderer_fb_impl.h>
#include <ref_fb_module/video_player_fb_impl.h>
#endif
#include <ref_fb_module/scaling_fb_impl.h>
#include <ref_fb_module/statistics_fb_impl.h>
#include <ref_fb_module/trigger_fb_impl.h>
#include <ref_fb_module/fft_fb_impl.h>
#include <ref_fb_module/version.h>
#include <ref_fb_module/power_reader_fb_impl.h>
#include <ref_fb_module/struct_decoder_fb_impl.h>
#include <ref_fb_module/time_delay_fb_impl.h>
#include <ref_fb_module/sum_reader_fb_impl.h>

BEGIN_NAMESPACE_REF_FB_MODULE

RefFBModule::RefFBModule(ContextPtr ctx)
    : Module("ReferenceFunctionBlockModule",
             daq::DevelopmentVersionInfo(
                REF_FB_MODULE_MAJOR_VERSION,
                REF_FB_MODULE_MINOR_VERSION,
                REF_FB_MODULE_PATCH_VERSION,
                0
             ),
             std::move(ctx),
             "ReferenceFunctionBlockModule")
{
}

DictPtr<IString, IFunctionBlockType> RefFBModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

#ifdef OPENDAQ_ENABLE_RENDERER
    const auto typeRenderer = Renderer::RendererFbImpl::CreateType(moduleInfo);
    types.set(typeRenderer.getId(), typeRenderer);
    const auto typeVideoPlayer = VideoPlayer::VideoPlayerFbImpl::CreateType(moduleInfo);
    types.set(typeVideoPlayer.getId(), typeVideoPlayer);
#endif

    const auto typeStatistics = Statistics::StatisticsFbImpl::CreateType(moduleInfo);
    types.set(typeStatistics.getId(), typeStatistics);

    const auto typePower = Power::PowerFbImpl::CreateType(moduleInfo);
    types.set(typePower.getId(), typePower);

    const auto typeScaling = Scaling::ScalingFbImpl::CreateType(moduleInfo);
    types.set(typeScaling.getId(), typeScaling);

    const auto typeClassifier = Classifier::ClassifierFbImpl::CreateType(moduleInfo);
    types.set(typeClassifier.getId(), typeClassifier);

    const auto typeTrigger = Trigger::TriggerFbImpl::CreateType(moduleInfo);
    types.set(typeTrigger.getId(), typeTrigger);

    const auto typeFFT = FFT::FFTFbImpl::CreateType(moduleInfo);
    types.set(typeFFT.getId(), typeFFT);

    const auto typePowerReader = PowerReader::PowerReaderFbImpl::CreateType(moduleInfo);
    types.set(typePowerReader.getId(), typePowerReader);

    const auto typeSumReader = SumReader::SumReaderFbImpl::CreateType();
    types.set(typeSumReader.getId(), typeSumReader);

    const auto typeStructDecoder = StructDecoder::StructDecoderFbImpl::CreateType(moduleInfo);
    types.set(typeStructDecoder.getId(), typeStructDecoder);

    const auto timeScaler = TimeDelay::TimeDelayFbImpl::CreateType();
    types.set(timeScaler.getId(), timeScaler);

    return types;
}

FunctionBlockPtr RefFBModule::onCreateFunctionBlock(const StringPtr& id,
                                                    const ComponentPtr& parent,
                                                    const StringPtr& localId,
                                                    const PropertyObjectPtr& config)
{
#ifdef OPENDAQ_ENABLE_RENDERER
    if (id == Renderer::RendererFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Renderer::RendererFbImpl>(moduleInfo, context, parent, localId, config);
        return fb;
    }
    if (id == VideoPlayer::VideoPlayerFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, VideoPlayer::VideoPlayerFbImpl>(moduleInfo, context, parent, localId, config);
        return fb;
    }
#endif
    if (id == Statistics::StatisticsFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Statistics::StatisticsFbImpl>(moduleInfo, context, parent, localId, config);
        return fb;
    }
    if (id == Power::PowerFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Power::PowerFbImpl>(moduleInfo, context, parent, localId);
        return fb;
    }
    if (id == Scaling::ScalingFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Scaling::ScalingFbImpl>(moduleInfo, context, parent, localId);
        return fb;
    }
    if (id == Classifier::ClassifierFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Classifier::ClassifierFbImpl>(moduleInfo, context, parent, localId);
        return fb;
    }
    if (id == Trigger::TriggerFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, Trigger::TriggerFbImpl>(moduleInfo, context, parent, localId, config);
        return fb;
    }
    if (id == FFT::FFTFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, FFT::FFTFbImpl>(moduleInfo, context, parent, localId);
        return fb;
    }
    if (id == PowerReader::PowerReaderFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, PowerReader::PowerReaderFbImpl>(moduleInfo, context, parent, localId);
        return fb;
    }
    if (id == SumReader::SumReaderFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, SumReader::SumReaderFbImpl>(context, parent, localId, config);
        return fb;
    }
    if (id == StructDecoder::StructDecoderFbImpl::CreateType(moduleInfo).getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, StructDecoder::StructDecoderFbImpl>(moduleInfo, context, parent, localId);
        return fb;
    }
    if (id == TimeDelay::TimeDelayFbImpl::CreateType().getId())
    {
        FunctionBlockPtr fb = createWithImplementation<IFunctionBlock, TimeDelay::TimeDelayFbImpl>(context, parent, localId, config);
        return fb;
    }

    LOG_W("Function block \"{}\" not found", id);
    DAQ_THROW_EXCEPTION(NotFoundException, "Function block not found");
}

END_NAMESPACE_REF_FB_MODULE
