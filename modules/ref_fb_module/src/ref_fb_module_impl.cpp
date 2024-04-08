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

BEGIN_NAMESPACE_REF_FB_MODULE

RefFbModule::RefFbModule(ContextPtr ctx)
    : Module("Reference function block module",
             daq::VersionInfo(REF_FB_MODULE_MAJOR_VERSION, REF_FB_MODULE_MINOR_VERSION, REF_FB_MODULE_PATCH_VERSION),
             std::move(ctx),
             "ReferenceFunctionBlock")
{
}

DictPtr<IString, IFunctionBlockType> RefFbModule::onGetAvailableFunctionBlockTypes()
{
    auto types = Dict<IString, IFunctionBlockType>();

#ifdef OPENDAQ_ENABLE_RENDERER
    auto typeRenderer = Renderer::RendererFbImpl::CreateType();
    types.set(typeRenderer.getId(), typeRenderer);
#endif

    auto typeStatistics = Statistics::StatisticsFbImpl::CreateType();
    types.set(typeStatistics.getId(), typeStatistics);

    auto typePower = Power::PowerFbImpl::CreateType();
    types.set(typePower.getId(), typePower);

    auto typeScaling = Scaling::ScalingFbImpl::CreateType();
    types.set(typeScaling.getId(), typeScaling);

    auto typeClassifier = Classifier::ClassifierFbImpl::CreateType();
    types.set(typeClassifier.getId(), typeClassifier);

    auto typeTrigger = Trigger::TriggerFbImpl::CreateType();
    types.set(typeTrigger.getId(), typeTrigger);

    auto typeFFT = FFT::FFTFbImpl::CreateType();
    types.set(typeFFT.getId(), typeFFT);

    return types;
}

FunctionBlockPtr RefFbModule::onCreateFunctionBlock(const StringPtr& id,
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


    LOG_W("Function block \"{}\" not found", id);
    throw NotFoundException("Function block not found");
}

END_NAMESPACE_REF_FB_MODULE
