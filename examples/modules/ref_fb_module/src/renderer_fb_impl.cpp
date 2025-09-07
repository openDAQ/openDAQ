#include <opendaq/function_block_ptr.h>
#include <ref_fb_module/arial.ttf.h>
#include <ref_fb_module/renderer_fb_impl.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/custom_log.h>
#include <ref_fb_module/dispatch.h>
#include <coreobjects/eval_value_factory.h>
#include <opendaq/thread_name.h>
#include <date/date.h>
#include <iomanip>
#include <opendaq/work_factory.h>
#include <coreobjects/callable_info_factory.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Renderer
{

static std::chrono::system_clock::duration TimeValueToDuration(Float timeValue)
{
    auto floatDuration = std::chrono::duration<float>(timeValue);
    return std::chrono::round<std::chrono::system_clock::duration>(floatDuration);
}

void SignalContext::processPacket()
{
    const auto conn = inputPort.getConnection();
    if (!conn.assigned())
        return;

    PacketPtr packet = conn.dequeue();
    while (packet.assigned())
    {
        if (packet.getType() == PacketType::Event)
            processEventPacket(packet);
        else if (packet.getType() == PacketType::Data)
            processDataPacket(packet);
        packet = conn.dequeue();
    }
}

void SignalContext::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        // TODO handle Null-descriptor params ('Null' sample type descriptors)
        DataDescriptorPtr valueSignalDescriptor = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        DataDescriptorPtr domainSignalDescriptor = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        this->handleDataDescriptorChanged(valueSignalDescriptor);
        this->handleDomainDescriptorChanged(domainSignalDescriptor);
    }
}

void SignalContext::processDataPacket(const DataPacketPtr& packet)
{
    if (!valid)
        return;

    auto domainPacket = packet.getDomainPacket();
    if (!domainPacket.assigned())
    {
        // LOGP_W("Packet recieved, but no domain packet assigned. Packet ignored")
        return;
    }

    if (ctx->freezed)
    {
        dataPacketsInFreezeMode.push_front(packet);
        while (dataPacketsInFreezeMode.size() > 1000)
            dataPacketsInFreezeMode.pop_back();
    }
    else
    {
        while (!dataPacketsInFreezeMode.empty())
        {
            auto pkt = dataPacketsInFreezeMode.back();
            dataPackets.push_front(pkt);
            dataPacketsInFreezeMode.pop_back();
        }
        dataPackets.push_front(packet);
        SAMPLE_TYPE_DISPATCH(domainSampleType, setLastDomainStamp, domainPacket)
    }
}

void SignalContext::setEpoch(std::string timeStr)
{
    origin = reader::parseEpoch(timeStr);
    hasTimeOrigin = true;
}

sf::Color SignalContext::getColor() const
{
    switch(this->index % 6)
    {
        case 0:
            return sf::Color::Red;
        case 1:
            return sf::Color::Yellow;
        case 2:
            return sf::Color::Green;
        case 3:
            return sf::Color::Magenta;
        case 4:
            return sf::Color::Cyan;
        case 5:
            return sf::Color::Blue;
        default:
            return sf::Color::White;
    }
}

void SignalContext::setCaption(const std::string& caption)
{
    if (caption.empty())
    {
        const auto sig = this->inputPort.getSignal();
        if (sig.assigned())
            this->caption = sig.getName().toStdString();
        else
            this->caption = "N/A";
    }
    else
    {
        this->caption = caption;
    }

    const auto unit = inputDataSignalDescriptor.getUnit();
    if (unit.assigned() && !unit.getSymbol().toStdString().empty())
        this->caption += fmt::format(" [{}]", unit.getSymbol().toStdString());
}

void SignalContext::handleDataDescriptorChanged(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return;

    this->inputDataSignalDescriptor = descriptor;

    if (descriptor.getDimensions().getCount() > 1)  // matrix not supported on the input
    {
        // logAndSetFutureComponentStatus(ComponentStatus::Warning, "Matrix signals not supported");
        return;
    }
    this->sampleType = descriptor.getSampleType();

    const auto valueRange = descriptor.getValueRange();
    if (!valueRange.assigned())
    {
        this->min = 0;
        this->max = 1;
    }
    else
    {
        this->min = valueRange.getLowValue();
        this->max = valueRange.getHighValue();
    }

    this->setCaption();
    this->valid = true;
}

void SignalContext::handleDomainDescriptorChanged(const DataDescriptorPtr& descriptor)
{
    if (!descriptor.assigned())
        return;

    this->inputDomainDataDescriptor = descriptor;
    this->valid = false;

    if (!descriptor.getTickResolution().assigned())
    {
        // LOGP_W("Domain resolution not assigned")
        return;
    }

    auto domainRes = descriptor.getTickResolution();
    if (domainRes.assigned())
    {
        this->domainResNum = domainRes.getNumerator();
        this->domainResDen = domainRes.getDenominator();
        Int gcd = std::gcd(this->domainResDen, this->domainResNum);
        this->domainResNum /= gcd;
        this->domainResDen /= gcd;
    }
    else
    {
        this->domainResNum = 1;
        this->domainResDen = 1;
    }

    const auto domainRule = descriptor.getRule();
    if (domainRule.getType() == DataRuleType::Linear)
    {
        const auto domainRuleParams = domainRule.getParameters();
        try
        {
            this->domainDelta = domainRuleParams.get("delta");
            this->domainStart = domainRuleParams.get("start");
        }
        catch (const std::exception& e)
        {
            DAQ_THROW_EXCEPTION(InvalidPropertyException, "Invalid data rule parameters: {}", e.what());
        }
        this->isExplicit = false;
    }
    else
    {
        this->durationInTicks = static_cast<int64_t>(ctx->duration / static_cast<double>(descriptor.getTickResolution()));
        this->isRange = descriptor.getSampleType() == SampleType::RangeInt64;
        this->isExplicit = true;
    }

    this->domainUnit = descriptor.getUnit().getSymbol().toStdString();
    this->domainQuantity = descriptor.getUnit().getQuantity().toStdString();
    this->domainSampleType = descriptor.getSampleType();

    const auto origin = descriptor.getOrigin();
    if (origin.assigned() && !origin.toStdString().empty())
    {
        try
        {
            if (this->domainUnit != "s" || this->domainQuantity != "time")
            {
            //    LOGP_W("Domain signal not time, origin ignored")
            }
            else
            {
                this->setEpoch(origin);
            //    LOG_D("Origin={}", date::format("%F %T", this->.origin))

                int64_t n = this->domainResNum * std::chrono::system_clock::time_point::period::den;
                int64_t d = this->domainResDen * std::chrono::system_clock::time_point::period::num;

                int64_t gcd_n_d = std::gcd(n, d);
                n /= gcd_n_d;
                d /= gcd_n_d;

                this->domainToTimeNum = n;
                this->domainToTimeDen = d;
            }
        }
        catch (...)
        {
            // logAndSetFutureComponentStatus(ComponentStatus::Warning, fmt::format("Invalid origin, ignored: {}", origin));
        }
    }
    this->valid = true;
}

template <SampleType DST>
void SignalContext::setLastDomainStamp(const DataPacketPtr& domainPacket)
{ 
    using SourceDomainType = typename SampleTypeToType<DST>::Type;
    using DestDomainType = typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type;

    // const auto domainDataDescriptor = domainPacket.getDataDescriptor();
    const auto sampleCount = domainPacket.getSampleCount();

    DestDomainType lastDomainStamp;
    if (this->isExplicit)
    {
        const auto domainDataPtr = static_cast<SourceDomainType*>(domainPacket.getData());
        lastDomainStamp = static_cast<DestDomainType>(*(domainDataPtr + sampleCount - 1));
    }
    else
    {
        NumberPtr offset = 0;
        if (domainPacket.getOffset().assigned())
            offset = domainPacket.getOffset();
        lastDomainStamp = static_cast<DestDomainType>(offset + (sampleCount - 1) * this->domainDelta +
                                                      this->domainStart); 
    }

    this->lastDomainStamp = lastDomainStamp;
    const DestDomainType domainDuration = ctx->duration * this->domainResDen / this->domainResNum;
    this->firstDomainStamp = lastDomainStamp - domainDuration;

    if (this->hasTimeOrigin)
    {
        timestampToTimePoint<DomainTypeCast<DST>::DomainSampleType>(lastDomainStamp);
    }
}

template <SampleType DomainSampleType>
void SignalContext::timestampToTimePoint(typename SampleTypeToType<DomainSampleType>::Type timeStamp)
{
    static_assert(DomainSampleType == SampleType::Float64 || DomainSampleType == SampleType::Int64 ||
                  DomainSampleType == SampleType::UInt64);

    if constexpr (std::is_floating_point_v<typename SampleTypeToType<DomainSampleType>::Type>)
    {
        this->lastTimeValue = this->origin + TimeValueToDuration(timeStamp);
    }
    else
    {
        this->lastTimeValue = this->origin + std::chrono::system_clock::time_point::duration(
            timeStamp * this->domainToTimeNum / this->domainToTimeDen);
    }
    this->firstTimeValue = this->lastTimeValue - TimeValueToDuration(ctx->duration);
}

template <SampleType DST>
void SignalContext::setSingleXAxis(std::chrono::system_clock::time_point lastTimeValue, Float lastDomainValue)
{
    using DestDomainType = typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type;

    if (ctx->hasTimeOrigin)
    {
        auto firstTimeValue = lastTimeValue - TimeValueToDuration(ctx->duration);
        this->singleAxisLastDomainStamp = static_cast<DestDomainType>(
            lastTimeValue.time_since_epoch().count() * this->domainToTimeDen / this->domainToTimeNum);
        this->singleAxisFirstDomainStamp = static_cast<DestDomainType>(
            firstTimeValue.time_since_epoch().count() * this->domainToTimeDen / this->domainToTimeNum);
    }
    else
    {
        auto firstDomainValue = lastDomainValue - ctx->duration;
        this->singleAxisLastDomainStamp =
            static_cast<DestDomainType>(lastDomainValue * this->domainResDen / this->domainResNum);
        this->singleAxisFirstDomainStamp =
            static_cast<DestDomainType>(firstDomainValue * this->domainResDen / this->domainResNum);
    }
}

RendererFbImpl::RendererFbImpl(const ContextPtr& ctx, 
                               const ComponentPtr& parent, 
                               const StringPtr& localId,
                               const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , rendererStopRequested(false)
    , resolutionChangedFlag(false)
    , signalContextIndex(0)
    , inputPortCount(0)
    , axisColor(150, 150, 150)
    , futureComponentStatus(ComponentStatus::Ok)
    , futureComponentMessage("")
{
    if (context.getScheduler().isMainLoopSet())
        if (config.assigned() && config.hasProperty("UseMainLoopForRenderer"))
            rendererUsesMainLoop = config.getPropertyValue("UseMainLoopForRenderer");

#ifdef __APPLE__
    if (!rendererUsesMainLoop)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Renderer function block requires the main loop to be set on scheduler for macOS.");
#endif

    initComponentStatus();
    initProperties();
    updateInputPorts();
    initializeRenderer();
}

void RendererFbImpl::rendererStopRequesteding()
{
    if (!rendererStopRequested)
    {
        rendererStopRequested = true;
        cv.notify_one();
    }
}

RendererFbImpl::~RendererFbImpl()
{
    rendererStopRequesteding();

    if (renderThread.joinable())
        renderThread.join();
    LOGP_D("Render thread stopped")
}

FunctionBlockTypePtr RendererFbImpl::CreateType()
{
    auto defaultConfig = PropertyObject();
#ifdef __APPLE__
    defaultConfig.addProperty(BoolProperty("UseMainLoopForRenderer", true));
#else
    defaultConfig.addProperty(BoolProperty("UseMainLoopForRenderer", false));
#endif

    return FunctionBlockType(
        "RefFBModuleRenderer",
        "Renderer",
        "Signal visualization",
        defaultConfig
    );
}

void RendererFbImpl::removed()
{
    rendererStopRequesteding();

    FunctionBlockImpl::removed();
}

void RendererFbImpl::initProperties()
{
    auto onPropertyValueWrite = [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    const auto durationProp = FloatPropertyBuilder("Duration", 1.0).setSuggestedValues(List<Float>(0.01, 0.1, 1.0, 10.0)).build();
    objPtr.addProperty(durationProp);
    objPtr.getOnPropertyValueWrite("Duration") += onPropertyValueWrite;

    const auto singleXAxisProp = BoolProperty("SingleXAxis", False);
    objPtr.addProperty(singleXAxisProp);
    objPtr.getOnPropertyValueWrite("SingleXAxis") += onPropertyValueWrite;

    const auto singleYAxisProp = BoolPropertyBuilder("SingleYAxis", False).setVisible(EvalValue("$SingleXAxis")).build();
    objPtr.addProperty(singleYAxisProp);
    objPtr.getOnPropertyValueWrite("SingleYAxis") += onPropertyValueWrite;

    const auto lineThicknessProp = FloatProperty("LineThickness", 1.0);
    objPtr.addProperty(lineThicknessProp);
    objPtr.getOnPropertyValueWrite("LineThickness") += onPropertyValueWrite;

    const auto freezeProp = BoolProperty("Freeze", False);
    objPtr.addProperty(freezeProp);
    objPtr.getOnPropertyValueWrite("Freeze") += onPropertyValueWrite;

    const auto resolutionProp =
        SelectionProperty("Resolution", List<IString>("640x480", "800x600", "1024x768", "1280x720", "1920x1080", "2560x1440"), 1);
    objPtr.addProperty(resolutionProp);
    objPtr.getOnPropertyValueWrite("Resolution") += [this](PropertyObjectPtr& /*obj*/, PropertyValueEventArgsPtr& /*args*/)
    {
        resolutionChanged();
    };

    const auto showLastValueProp = BoolProperty("ShowLastValue", False);
    objPtr.addProperty(showLastValueProp);
    objPtr.getOnPropertyValueWrite("ShowLastValue") += onPropertyValueWrite;

    const auto useCustomMinMaxValue = BoolProperty("UseCustomMinMaxValue", False);
    objPtr.addProperty(useCustomMinMaxValue);
    objPtr.getOnPropertyValueWrite("UseCustomMinMaxValue") += onPropertyValueWrite;

    const auto customHighValueProp = FloatPropertyBuilder("CustomMaxValue", 10.0).setVisible(EvalValue("$UseCustomMinMaxValue")).build();
    objPtr.addProperty(customHighValueProp);
    objPtr.getOnPropertyValueWrite("CustomMaxValue") += onPropertyValueWrite;

    const auto customLowValueProp = FloatPropertyBuilder("CustomMinValue", -10.0).setVisible(EvalValue("$UseCustomMinMaxValue")).build();
    objPtr.addProperty(customLowValueProp);
    objPtr.getOnPropertyValueWrite("CustomMinValue") += onPropertyValueWrite;

    const auto useCustom2dRangeValueProp = BoolPropertyBuilder("UseCustom2dRangeValue", false).build();
    objPtr.addProperty(useCustom2dRangeValueProp);
    objPtr.getOnPropertyValueWrite("UseCustom2dRangeValue") += onPropertyValueWrite;

    const auto custom2dMinRangeProp = IntPropertyBuilder("Custom2dMinRange", 0).setVisible(EvalValue("$UseCustom2dRangeValue")).build();
    objPtr.addProperty(custom2dMinRangeProp);
    objPtr.getOnPropertyValueWrite("Custom2dMinRange") += onPropertyValueWrite;

    const auto custom2dMaxRangeProp = IntPropertyBuilder("Custom2dMaxRange", 100).setVisible(EvalValue("$UseCustom2dRangeValue")).build();
    objPtr.addProperty(custom2dMaxRangeProp);
    objPtr.getOnPropertyValueWrite("Custom2dMaxRange") += onPropertyValueWrite;

    objPtr.addProperty(FunctionPropertyBuilder("OpenWindow", ProcedureInfo()).setReadOnly(true).build());
    objPtr.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("OpenWindow", Procedure([this]
    {
        startRendererWindow();
    }));

    readProperties();
    readResolutionProperty();
}

void RendererFbImpl::propertyChanged()
{
    readProperties();
}

void RendererFbImpl::resolutionChanged()
{
    readResolutionProperty();
    resolutionChangedFlag = true;
}

void RendererFbImpl::readProperties()
{
    ctx.duration = objPtr.getPropertyValue("Duration");
    LOG_T("Properties: Duration {}", ctx.duration)
    singleXAxis = objPtr.getPropertyValue("SingleXAxis");
    LOG_T("Properties: SingleXAxis {}", singleXAxis)
    singleYAxis = objPtr.getPropertyValue("SingleYAxis");
    LOG_T("Properties: SingleYAxis {}", singleYAxis)
    ctx.freezed = objPtr.getPropertyValue("Freeze");
    LOG_T("Properties: Freeze {}", ctx.freezed)
    showLastValue = objPtr.getPropertyValue("ShowLastValue");
    LOG_T("Properties: ShowLastValue {}", showLastValue)
    lineThickness = objPtr.getPropertyValue("LineThickness");
    LOG_T("Properties: LineThickness {}", lineThickness)

    useCustomMinMaxValue = objPtr.getPropertyValue("UseCustomMinMaxValue");
    LOG_T("Properties: UseCustomMinMaxValue {}", useCustomMinMaxValue)
    customMinValue = objPtr.getPropertyValue("CustomMinValue");
    LOG_T("Properties: CustomMinValue {}", customMinValue)
    customMaxValue = objPtr.getPropertyValue("CustomMaxValue");
    LOG_T("Properties: CustomMaxValue {}", customMaxValue)

    useCustom2dRangeValue = objPtr.getPropertyValue("UseCustom2dRangeValue");
    LOG_T("Properties: UseCustom2dRangeValue {}", useCustom2dRangeValue);
    custom2dMinRange = objPtr.getPropertyValue("Custom2dMinRange");
    custom2dMaxRange = objPtr.getPropertyValue("Custom2dMaxRange");

    LOG_T("Properties: Custom2dMinRange {}", custom2dMinRange);
    LOG_T("Properties: Custom2dMaxRange {}", custom2dMaxRange);
    if (custom2dMinRange > custom2dMaxRange)
    {
        setComponentStatusWithMessage(ComponentStatus::Error, "Property custom2dMaxRange has to be more than custom2dMinRange");
    }
    else
    {
        setComponentStatus(ComponentStatus::Ok);
    }
        
}

void RendererFbImpl::readResolutionProperty()
{
    resolution = objPtr.getPropertyValue("Resolution");
    LOG_T("Properties: Resolution {}", resolution)
}

void RendererFbImpl::logAndSetFutureComponentStatus(ComponentStatus status, StringPtr message)
{
    if (status == ComponentStatus::Warning)
    {
        LOG_W("{}", message)
    }
    else if (status == ComponentStatus::Error)
    {
        LOG_E("{}", message)
    }
    else
    {
        LOG_I("{}", message)
    }

    futureComponentStatus = status;
    futureComponentMessage = message;
}

void RendererFbImpl::updateInputPorts()
{
    for (auto it = signalContexts.begin(); it != signalContexts.end();)
    {
        if (!it->inputPort.getSignal().assigned())
        {
            removeInputPort(it->inputPort);
            it = signalContexts.erase(it);
        }
        else
        {
            ++it;
        }
    }

    const auto inputPort = createAndAddInputPort(fmt::format("Input{}", inputPortCount++), PacketReadyNotification::SameThread);

    signalContexts.emplace_back(SignalContext{ 0, inputPort, &ctx});
    for (size_t i = 0; i < signalContexts.size(); i++)
        signalContexts[i].index = i;
}

void RendererFbImpl::renderSignals(sf::RenderTarget& renderTarget, const sf::Font& renderFont)
{
    for (auto& sigCtx : signalContexts)
    {
        if (sigCtx.valid)
            SAMPLE_TYPE_DISPATCH(sigCtx.domainSampleType, renderSignal, sigCtx, renderTarget, renderFont);
    }
}

template <SampleType DST>
void RendererFbImpl::renderPacket(
    SignalContext& signalContext,
    sf::RenderTarget& renderTarget,
    const sf::Font& renderFont,
    const DataPacketPtr& packet,
    bool& havePrevPacket,
    typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type& nextExpectedDomainPacketValue,
    std::unique_ptr<Polyline>& line,
    bool& end)
{
    const auto domainPacket = packet.getDomainPacket();
    if (domainPacket.getSampleCount() == 0)
        return;

    const auto domainDataDescriptor = signalContext.inputDomainDataDescriptor;
    const auto domainRule = domainDataDescriptor.getRule();
    const size_t signalDimension = signalContext.inputDataSignalDescriptor.getDimensions().getCount();
    
    if (signalDimension == 1)
        renderArrayPacketImplicitAndExplicit<DST>(signalContext, domainRule.getType(), renderTarget, renderFont, packet, havePrevPacket, nextExpectedDomainPacketValue, line, end);
    else
        renderPacketImplicitAndExplicit<DST>(signalContext, domainRule.getType(), renderTarget, renderFont, packet, havePrevPacket, nextExpectedDomainPacketValue, line, end);
}

template <SampleType DST>
void RendererFbImpl::renderPacketImplicitAndExplicit(
    SignalContext& signalContext,
    DataRuleType domainRuleType,
    sf::RenderTarget& renderTarget,
    const sf::Font& renderFont,
    const DataPacketPtr& packet,
    bool& havePrevPacket,
    typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type& nextExpectedDomainPacketValue,
    std::unique_ptr<Polyline>& line,
    bool& end)
{
    using SourceDomainType = typename SampleTypeToType<DST>::Type;
    using DestDomainType = typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type;

    const float xSize = signalContext.bottomRight.x - signalContext.topLeft.x;
    const float xOffset = signalContext.topLeft.x;
    const float ySize = signalContext.bottomRight.y - signalContext.topLeft.y;
    const float yOffset = signalContext.bottomRight.y;

    bool gap;
    auto domainPacket = packet.getDomainPacket();
    auto domainDataDescriptor = domainPacket.getDataDescriptor();
    SizeT domainPacketSampleCount{};
    const auto domainRule = domainDataDescriptor.getRule();
    const auto domainRuleParams = domainRule.getParameters();
    const auto domainOffset = domainPacket.getOffset();
    const auto samplesInPacket = packet.getSampleCount();
    DestDomainType curDomainPacketValue{};
    DestDomainType firstDomainPacketValue{};
    DestDomainType delta{};
    DestDomainType start{};
    SourceDomainType* domainData{};

    if (domainRuleType == DataRuleType::Linear)
    {
        firstDomainPacketValue = domainPacket.getOffset();
        delta = domainRuleParams.get("delta");
        start = domainRuleParams.get("start");
        curDomainPacketValue = firstDomainPacketValue + static_cast<DestDomainType>(samplesInPacket - 1) * delta + start;
        gap = havePrevPacket && curDomainPacketValue != nextExpectedDomainPacketValue;
        nextExpectedDomainPacketValue = firstDomainPacketValue - delta + start;
    }
    else
    {
        domainPacketSampleCount = domainPacket.getSampleCount();
        domainData = static_cast<SourceDomainType*>(domainPacket.getData());
        domainData += domainPacket.getSampleCount() - 1;
        curDomainPacketValue = static_cast<DestDomainType>(*domainData);
        gap = havePrevPacket && domainPacketSampleCount == 0;
    }

    DestDomainType firstDomainValue;
    DestDomainType lastDomainValue;
    if (singleXAxis)
    {
        firstDomainValue = std::get<DestDomainType>(signalContext.singleAxisFirstDomainStamp);
        lastDomainValue = std::get<DestDomainType>(signalContext.singleAxisLastDomainStamp);
    }
    else
    {
        firstDomainValue = std::get<DestDomainType>(signalContext.firstDomainStamp);
        lastDomainValue = std::get<DestDomainType>(signalContext.lastDomainStamp);
    }

    auto domainFactor = (lastDomainValue - firstDomainValue) / static_cast<double>(xSize);

    double yMax, yMin;
    getYMinMax(signalContext, yMax, yMin);

    auto valueFactor = (yMax - yMin) / static_cast<double>(ySize);

    if (gap)
    {
        renderTarget.draw(*line);
        line = std::make_unique<Polyline>(lineThickness, LineStyle::solid);
        line->setColor(signalContext.getColor());
    }

    havePrevPacket = true;

    if (domainRuleType == DataRuleType::Explicit && domainPacketSampleCount == 0)
        return;

    size_t sampleSize = getSampleSize(signalContext.sampleType);
    auto data = reinterpret_cast<uint8_t*>(packet.getData()) + samplesInPacket * sampleSize;

    size_t i = 0;
    while (i < samplesInPacket)
    {
        if (curDomainPacketValue < firstDomainValue)
            break;

        data -= sampleSize;
        double value;
        switch (signalContext.sampleType)
        {
            case (SampleType::Float32):
                value = *(reinterpret_cast<float*>(data));
                break;
            case (SampleType::Float64):
                value = *(reinterpret_cast<double*>(data));
                break;
            case (SampleType::UInt8):
                value = *(reinterpret_cast<uint8_t*>(data));
                break;
            case (SampleType::Int8):
                value = *(reinterpret_cast<int8_t*>(data));
                break;
            case (SampleType::UInt16):
                value = *(reinterpret_cast<uint16_t*>(data));
                break;
            case (SampleType::Int16):
                value = *(reinterpret_cast<int16_t*>(data));
                break;
            case (SampleType::UInt32):
                value = *(reinterpret_cast<uint32_t*>(data));
                break;
            case (SampleType::Int32):
                value = *(reinterpret_cast<int32_t*>(data));
                break;
            case (SampleType::UInt64):
                value = *(reinterpret_cast<uint64_t*>(data));
                break;
            case (SampleType::Int64):
                value = *(reinterpret_cast<int64_t*>(data));
                break;
            default:
                value = 0.0;
        }
        if (!signalContext.lastValueSet)
        {
            signalContext.lastValue = value;
            signalContext.lastValueSet = true;
        }

        float xPos = xOffset + static_cast<float>(1.0 * (curDomainPacketValue - firstDomainValue) / domainFactor);
        float yPos = yOffset - static_cast<float>((value - yMin) / valueFactor);

        if (yPos < signalContext.topLeft.y)
            yPos = signalContext.topLeft.y;
        else if (yPos > signalContext.bottomRight.y)
            yPos = signalContext.bottomRight.y;

        line->addPoint(xPos, yPos);
        if (domainRuleType == DataRuleType::Linear)
            curDomainPacketValue -= delta;
        else
            curDomainPacketValue = static_cast<DestDomainType>(*(--domainData));
        i++;
    }
    if (i == 0)
        end = true;
}

template <SampleType DST>
void RendererFbImpl::renderArrayPacketImplicitAndExplicit(
    SignalContext& signalContext,
    DataRuleType domainRuleType,
    sf::RenderTarget& renderTarget,
    const sf::Font& renderFont,
    const DataPacketPtr& packet,
    bool& havePrevPacket,
    typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type& nextExpectedDomainPacketValue,
    std::unique_ptr<Polyline>& line,
    bool& end)
{
    const float xSize = signalContext.bottomRight.x - signalContext.topLeft.x;
    const float xOffset = signalContext.topLeft.x;
    const float ySize = signalContext.bottomRight.y - signalContext.topLeft.y;
    const float yOffset = signalContext.bottomRight.y;

    auto domainPacket = packet.getDomainPacket();
    auto domainDataDescriptor = domainPacket.getDataDescriptor();
    const auto samplesInPacket = packet.getSampleCount();
    
    size_t xTickOffset = 0;
    size_t xTickCount = signalContext.inputDataSignalDescriptor.getDimensions()[0].getSize();

    if (useCustom2dRangeValue && (custom2dMinRange < custom2dMaxRange))
    {
        if (custom2dMinRange < xTickCount)
            xTickOffset = custom2dMinRange;
        if (custom2dMaxRange + 1 <= xTickCount)
            xTickCount = custom2dMaxRange + 1;
        xTickCount -= xTickOffset;
    }
    
    double domainFactor = static_cast<double>(xTickCount - 1) / static_cast<double>(xSize);

    double yMax, yMin;
    getYMinMax(signalContext, yMax, yMin);

    auto valueFactor = (yMax - yMin) / static_cast<double>(ySize);

    end = true;
    havePrevPacket = false;
    auto data = reinterpret_cast<uint8_t*>(packet.getData());

    if (samplesInPacket == 0)
        return;

    double value;
    for (size_t i = 0; i < xTickCount; i++) 
    {
        size_t idx = i + xTickOffset;
        switch (signalContext.sampleType)
        {
            case (SampleType::Float32):
                value = reinterpret_cast<float*>(data)[idx];
                break;
            case (SampleType::Float64):
                value = reinterpret_cast<double*>(data)[idx];
                break;
            case (SampleType::UInt8):
                value = reinterpret_cast<uint8_t*>(data)[idx];
                break;
            case (SampleType::Int8):
                value = reinterpret_cast<int8_t*>(data)[idx];
                break;
            case (SampleType::UInt16):
                value = reinterpret_cast<uint16_t*>(data)[idx];
                break;
            case (SampleType::Int16):
                value = reinterpret_cast<int16_t*>(data)[idx];
                break;
            case (SampleType::UInt32):
                value = reinterpret_cast<uint32_t*>(data)[idx];
                break;
            case (SampleType::Int32):
                value = reinterpret_cast<int32_t*>(data)[idx];
                break;
            case (SampleType::UInt64):
                value = reinterpret_cast<uint64_t*>(data)[idx];
                break;
            case (SampleType::Int64):
                value = reinterpret_cast<int64_t*>(data)[idx];
                break;
            default:
                value = 0.0;
        }

        float xPos = xOffset + static_cast<float>(1.0 * i / domainFactor);
        float yPos = yOffset - static_cast<float>((value - yMin) / valueFactor);

        if (yPos < signalContext.topLeft.y)
            yPos = signalContext.topLeft.y;
        else if (yPos > signalContext.bottomRight.y)
            yPos = signalContext.bottomRight.y;

        line->addPoint(xPos, yPos);
    }
}

template <SampleType DST>
void RendererFbImpl::renderSignal(SignalContext& signalContext, sf::RenderTarget& renderTarget, const sf::Font& renderFont)
{
    signalContext.lastValueSet = false;
    if (signalContext.dataPackets.empty())
        return;

    auto line = std::make_unique<Polyline>(lineThickness, LineStyle::solid);
    line->setColor(signalContext.getColor());

    typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type nextExpectedDomainPacketValue{};
    bool havePrevPacket = false;
    bool end = false;

    auto packetIt = signalContext.dataPackets.begin();
    for (; packetIt != signalContext.dataPackets.end(); ++packetIt)
    {
        renderPacket<DST>(signalContext, renderTarget, renderFont, *packetIt, havePrevPacket, nextExpectedDomainPacketValue, line, end);

        if (end)
            break;
    }

    // save last packet for vector signal to escape blinking
    // in case if there is no new packets - renderer will you this
    size_t signalDimension = signalContext.inputDataSignalDescriptor.getDimensions().getCount();
    if (signalDimension == 1 && packetIt != signalContext.dataPackets.end())
        packetIt++;

    signalContext.dataPackets.erase(packetIt, signalContext.dataPackets.end());

    renderTarget.draw(*line);

    if (signalContext.lastValueSet && showLastValue)
    {
        sf::Text lastValueText(renderFont);
        lastValueText.setFillColor(axisColor);
        lastValueText.setCharacterSize(16);
        std::ostringstream valueStr;
        if (singleXAxis && singleYAxis)
            valueStr << signalContext.caption;
        else
            valueStr << "Value";
        valueStr << " = " << std::fixed << std::showpoint << std::setprecision(3) << signalContext.lastValue;
        lastValueText.setString(valueStr.str());
        const auto valueBounds = lastValueText.getGlobalBounds();
        float top = signalContext.topLeft.y - valueBounds.size.y * 2.0f;
        if (singleXAxis && singleYAxis)
            top += signalContext.index * valueBounds.size.y * 2.0f;

        lastValueText.setPosition({signalContext.bottomRight.x - valueBounds.size.x, top});

        renderTarget.draw(lastValueText);
    }
}

void RendererFbImpl::onConnected(const InputPortPtr& inputPort)
{
    auto lock = this->getRecursiveConfigLock();

    subscribeToSignalCoreEvent(inputPort.getSignal());
    updateInputPorts();
    LOG_T("Connected to port {}", inputPort.getLocalId());
}

void RendererFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    auto lock = this->getRecursiveConfigLock();

    updateInputPorts();
    LOG_T("Disconnected from port {}", inputPort.getLocalId());
}

void RendererFbImpl::getWidthAndHeight(unsigned int& width, unsigned int& height)
{
    switch (resolution)
    {
        case 0:
            width = 640;
            height = 480;
            break;
        case 1:
            width = 800;
            height = 600;
            break;
        case 2:
            width = 1024;
            height = 768;
            break;
        case 3:
            width = 1280;
            height = 720;
            break;
        case 4:
            width = 1920;
            height = 1080;
            break;
        case 5:
            width = 2560;
            height = 1440;
            break;
    }
}

void RendererFbImpl::updateSingleXAxis() {
    // we can use singleXAxis only on the same dimension signals
    if (singleXAxis && signalContexts.size() > 2) 
    {
        auto firstSignalDimension = signalContexts.front().inputDataSignalDescriptor.getDimensions().getCount();
        for (auto sigCtx = signalContexts.begin() + 1; sigCtx != signalContexts.end() - 1; sigCtx++) 
        {
            auto curSignalDimension = sigCtx->inputDataSignalDescriptor.getDimensions().getCount();
            if (firstSignalDimension != curSignalDimension) 
            {
                singleXAxis = false;
                logAndSetFutureComponentStatus(ComponentStatus::Warning, "Renderer has multiple input signals with different dimension. Property singleXAxis is turned off");
                break;
            }
        }
    }
}

void RendererFbImpl::initializeRenderer()
{
    renderFont = std::make_unique<sf::Font>();
    if (!renderFont->openFromMemory(ARIAL_TTF, sizeof(ARIAL_TTF)))
    {
        setComponentStatusWithMessage(ComponentStatus::Error, "Failed to load renderFont");
        throw std::runtime_error("Failed to load renderFont");
    }

    defaultWaitTime = std::chrono::milliseconds(20);
    startRendererWindow();
}

void RendererFbImpl::startRendererWindow()
{
    if (window)
        return;

    unsigned int width;
    unsigned int height;
    getWidthAndHeight(width, height);
    window = std::make_unique<sf::RenderWindow>(sf::VideoMode({width, height}), "Renderer", sf::Style::Close | sf::Style::Titlebar);
    topLeft = sf::Vector2(0.0f, 0.0f);
    bottomRight = sf::Vector2f(width, height);

    waitTime = std::chrono::steady_clock::now() + defaultWaitTime;
    
    if (rendererUsesMainLoop)
    {
        LOGP_D("Using main thread for rendering")
        auto scheduler = context.getScheduler();
        auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();
        scheduler.scheduleWorkOnMainLoop(WorkRepetitive([this, thisWeakRef = std::move(thisWeakRef)]
        {
            const auto thisFb = thisWeakRef.getRef();
            if (!thisFb.assigned())
                return false;

            if (rendererStopRequested || !window)
                return false;

            if (std::chrono::steady_clock::now() < waitTime)
                return true;
            waitTime += defaultWaitTime;

            if (resolutionChangedFlag)
            {
                resolutionChangedFlag = false;
                resize(*window);
            }

            while (auto event = window->pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                {
                    window.reset();
                    return false;
                }
            }

            processSignalContexts();

            window->clear();

            updateSingleXAxis();

            if (singleXAxis)
                prepareSingleXAxis();
            renderAxes(*window, *renderFont);
            renderSignals(*window, *renderFont);

            window->display();

            setComponentStatusWithMessage(futureComponentStatus, futureComponentMessage);
            futureComponentStatus = ComponentStatus::Ok;
            futureComponentMessage = "";
            return true;
        }));
    }
    else 
    {
        LOGP_D("Using separate thread for rendering")
        (void)(window->setActive(false));
        renderThread = std::thread([this] 
        {
            (void)(window->setActive(true));
            renderLoop();
        });
    }
}

void RendererFbImpl::renderLoop()
{
    daqNameThread("Renderer");

    auto lock = getUniqueLock();
    while (!rendererStopRequested && window)
    {
        cv.wait_until(lock, waitTime);
        if (rendererStopRequested || !window)
            continue;

        waitTime += defaultWaitTime;

        if (resolutionChangedFlag)
        {
            resolutionChangedFlag = false;
            resize(*window);
        }

        while (auto event = window->pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.reset();
                break;
            }
        }

        processSignalContexts();

        window->clear();

        updateSingleXAxis();

        if (singleXAxis)
            prepareSingleXAxis();
        renderAxes(*window, *renderFont);
        renderSignals(*window, *renderFont);

        window->display();

        setComponentStatusWithMessage(futureComponentStatus, futureComponentMessage);
        futureComponentStatus = ComponentStatus::Ok;
        futureComponentMessage = "";
    }

    resolutionChangedFlag = false;
}

void RendererFbImpl::resize(sf::RenderWindow& window)
{
    unsigned int width;
    unsigned int height;
    getWidthAndHeight(width, height);
    topLeft = sf::Vector2(0.0f, 0.0f);
    bottomRight = sf::Vector2f(width, height);

    for (auto& sigCtx : signalContexts)
    {
        sigCtx.topLeft = topLeft;
        sigCtx.bottomRight = bottomRight;
    }

    window.setSize(sf::Vector2u(width, height));
    sf::FloatRect viewRect({0.0f, 0.0f}, {static_cast<float>(width), static_cast<float>(height)});
    window.setView(sf::View(viewRect));
}

void RendererFbImpl::processSignalContexts()
{
    for (auto& sigCtx: signalContexts)
        sigCtx.processPacket();
}

template <typename Iter, typename Cont>
bool RendererFbImpl::isLastIter(Iter iter, const Cont& cont)
{
    return (iter != cont.end()) && (std::next(iter) != cont.end()) && (std::next(iter, 2) == cont.end());
}

void RendererFbImpl::renderAxes(sf::RenderTarget& renderTarget, const sf::Font& renderFont)
{
    const float xAxisLabelHeight = 40.0f;

    if (singleYAxis && singleXAxis)
    {
        yMaxValue = std::numeric_limits<double>::min();
        yMinValue = std::numeric_limits<double>::max();

        for (auto sigIt = signalContexts.begin(); sigIt != signalContexts.end() - 1; ++sigIt)
        {
            if (!sigIt->valid)
                continue;
            sigIt->topLeft = sf::Vector2f(75.0f, xAxisLabelHeight);
            sigIt->bottomRight = sf::Vector2f(bottomRight.x - 25.0f, bottomRight.y - xAxisLabelHeight);
            if (sigIt->min < yMinValue)
                yMinValue = sigIt->min;
            if (sigIt->max > yMaxValue)
                yMaxValue = sigIt->max;

            if (isLastIter(sigIt, signalContexts))
                renderAxis(renderTarget, *sigIt, renderFont, true, false);
        }
        renderMultiTitle(renderTarget, renderFont);
    }
    else
    {
        const size_t axesCount = signalContexts.size() - 1;
        float itemHeight;
        if (singleXAxis)
            itemHeight = (bottomRight.y - topLeft.y - xAxisLabelHeight) / static_cast<float>(axesCount);
        else
            itemHeight = (bottomRight.y - topLeft.y) / static_cast<float>(axesCount);
        const float width = bottomRight.x - topLeft.x;
        float curTop = 0.0f;
        for (auto sigIt = signalContexts.begin(); sigIt != signalContexts.end() - 1; ++sigIt)
        {
            if (!sigIt->valid)
                continue;
            sigIt->topLeft = sf::Vector2f(75.0f, curTop + xAxisLabelHeight);
            curTop += itemHeight;
            float bottom = curTop;
            if (!singleXAxis)
                bottom -= xAxisLabelHeight;
            sigIt->bottomRight = sf::Vector2f(static_cast<float>(width) - 25.0f, bottom);
            const auto drawXAxisLabels = !singleXAxis || isLastIter(sigIt, signalContexts);
            renderAxis(renderTarget, *sigIt, renderFont, drawXAxisLabels, true);
        }
    }
}

template <SampleType DST>
void RendererFbImpl::domainStampToDomainValue(Float& lastDomainValue, const SignalContext& signalContext, DomainStamp domainStamp)
{
    using Type = typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type;
    lastDomainValue = std::get<Type>(domainStamp) * signalContext.domainResNum * 1.0 / signalContext.domainResDen;
}

void RendererFbImpl::prepareSingleXAxis()
{
    if (signalContexts.size() <= 1)
        return;

    singleXAxisConfigured = false;

    try
    {
        auto sigIt = signalContexts.begin();
        if (!sigIt->valid)
        {
            DAQ_THROW_EXCEPTION(InvalidStateException, "First signal not valid");
        }

        ctx.hasTimeOrigin = sigIt->hasTimeOrigin;
        SAMPLE_TYPE_DISPATCH(sigIt->domainSampleType, domainStampToDomainValue, lastDomainValue, *sigIt, sigIt->lastDomainStamp)
        lastTimeValue = sigIt->lastTimeValue;
        domainUnit = sigIt->domainUnit;
        domainQuantity = sigIt->domainQuantity;

        for (; sigIt != signalContexts.end() - 1; ++sigIt)
        {
            if (!sigIt->valid)
                continue;

            if (sigIt->hasTimeOrigin != ctx.hasTimeOrigin)
            {
                DAQ_THROW_EXCEPTION(InvalidStateException, "Time origin set on some signals, but not all of them");
            }

            if (!ctx.hasTimeOrigin)
            {
               if (domainUnit != sigIt->domainUnit)
               {
                    DAQ_THROW_EXCEPTION(InvalidStateException, "Domain unit not equal");
               }

               if (domainQuantity != sigIt->domainQuantity)
               {
                    DAQ_THROW_EXCEPTION(InvalidStateException, "Domain quantity not equal");
               }
            }

            if (ctx.hasTimeOrigin)
            {
               if (sigIt->lastTimeValue > lastTimeValue)
                   lastTimeValue = sigIt->lastTimeValue;
            }
            else
            {
               Float dValue{};
               SAMPLE_TYPE_DISPATCH(sigIt->domainSampleType, domainStampToDomainValue, dValue, *sigIt, sigIt->lastDomainStamp)
               if (dValue > lastDomainValue)
                   lastDomainValue = dValue;
            }
        }

        singleXAxisConfigured = true;
        for (sigIt = signalContexts.begin(); sigIt != signalContexts.end() - 1; ++sigIt)
        {
            SAMPLE_TYPE_DISPATCH(sigIt->domainSampleType, sigIt->setSingleXAxis, lastTimeValue, lastDomainValue)
        }

    }
    catch (const std::exception& e)
    {
        logAndSetFutureComponentStatus(ComponentStatus::Error, fmt::format("Unable to configure single X axis: {}", e.what()));
    }
}

void RendererFbImpl::getYMinMax(const SignalContext& signalContext, double& yMax, double& yMin)
{
    if (useCustomMinMaxValue)
    {
        yMax = customMaxValue;
        yMin = customMinValue;
    }
    else
    {
        if (singleXAxis && singleYAxis)
        {
            yMax = yMaxValue;
            yMin = yMinValue;
        }
        else
        {
            yMax = signalContext.max;
            yMin = signalContext.min;
        }
    }
}

void RendererFbImpl::renderAxis(sf::RenderTarget& renderTarget, SignalContext& signalContext, const sf::Font& renderFont, bool drawXAxisLabels, bool drawTitle)
{
    size_t yTickCount = 5;

    size_t xTickOffset = 0;
    size_t xTickStep = 1;
    size_t xTickCount = 5;

    daq::ListPtr<daq::IBaseObject> labels{};

    size_t signalDimension = signalContext.inputDataSignalDescriptor.getDimensions().getCount();

    // create label values for horizontal axi
    if (signalDimension == 1) 
    {
        auto domainDataDimension = signalContext.inputDataSignalDescriptor.getDimensions()[0];
        labels = domainDataDimension.getLabels();
        xTickCount = labels.getCount();
        if (useCustom2dRangeValue && (custom2dMinRange < custom2dMaxRange))
        {
            if (custom2dMinRange < labels.getCount())
                xTickOffset = custom2dMinRange;
            if (custom2dMaxRange + 1 <= labels.getCount())
                xTickCount = custom2dMaxRange + 1;
            xTickCount -= xTickOffset;
        }
        if (xTickCount > 11)
        {
            xTickStep = (xTickCount + 10) / 11;
        }
    }
    else if (signalContext.hasTimeOrigin)
    {
        labels = List<IString>();
        for (size_t i = 0; i < xTickCount; i += xTickStep)
        {
            if ((i & 1) == 1)
            {
                labels.pushBack(String(""));
            }
            else
            {
                const auto tp = signalContext.lastTimeValue - TimeValueToDuration(ctx.duration * (static_cast<double>(xTickCount - 1 - i) / static_cast<double>(xTickCount - 1)));
                const auto tpms = date::floor<std::chrono::milliseconds>(tp);
                labels.pushBack(String(date::format("%F %T", tpms)));
            }
        }
    }
    else
    {
        labels = List<IFloat>();
        for (size_t i = 0; i < xTickCount; i += xTickStep)
        {
            labels.pushBack(ctx.duration * (static_cast<double>(i) / static_cast<double>(xTickCount - 1)));
        }
    }

    // create left border line
    Polyline leftLine(lineThickness, LineStyle::solid);
    leftLine.setColor(axisColor);
    leftLine.addPoint(signalContext.topLeft);
    leftLine.addPoint(signalContext.topLeft.x, signalContext.bottomRight.y);

    renderTarget.draw(leftLine);

    // create bottom border line
    Polyline bottomLine(lineThickness, LineStyle::solid);
    bottomLine.setColor(axisColor);
    bottomLine.addPoint(signalContext.topLeft.x, signalContext.bottomRight.y);
    bottomLine.addPoint(signalContext.bottomRight);

    renderTarget.draw(bottomLine);

    // create grid
    const float xSize = signalContext.bottomRight.x - signalContext.topLeft.x;
    const float ySize = signalContext.bottomRight.y - signalContext.topLeft.y;

    // create horizontal grid
    for (size_t i = 1; i < yTickCount; i++)
    {
        const float yPos = signalContext.bottomRight.y - (1.0f * i * ySize / static_cast<float>(yTickCount - 1));

        Polyline imLineHorz(lineThickness, LineStyle::dash);
        imLineHorz.setColor(axisColor);
        imLineHorz.addPoint(signalContext.topLeft.x, yPos);
        imLineHorz.addPoint(signalContext.bottomRight.x, yPos);
        renderTarget.draw(imLineHorz);
    }

    // create horizontal grid
    for (size_t i = xTickStep; i < xTickCount; i += xTickStep)
    {
        const float xPos = signalContext.topLeft.x + (1.0f * i * xSize / static_cast<float>(xTickCount - 1));

        Polyline imLineVert(lineThickness, LineStyle::dash);
        imLineVert.setColor(axisColor);
        imLineVert.addPoint(xPos, signalContext.topLeft.y);
        imLineVert.addPoint(xPos, signalContext.bottomRight.y);
        renderTarget.draw(imLineVert);
    }

    // create labeles for vertical axi
    for (size_t i = 0; i < yTickCount; i++) 
    {
        const float yPos = signalContext.bottomRight.y - (1.0f * static_cast<float>(i) * ySize / static_cast<float>(yTickCount - 1));

        sf::Text valueText(renderFont);
        valueText.setFillColor(axisColor);
        valueText.setCharacterSize(16);
        std::ostringstream valueStr;

        double yMax, yMin;
        getYMinMax(signalContext, yMax, yMin);

        valueStr << std::fixed << std::showpoint << std::setprecision(2)
                 << yMin + (yMax - yMin) * (static_cast<double>(i) / static_cast<double>(yTickCount - 1));
        valueText.setString(valueStr.str());
        const auto valueBounds = valueText.getGlobalBounds();
        const auto d = valueText.getCharacterSize() / 2.0f;
        valueText.setPosition({signalContext.topLeft.x - valueBounds.size.x - d, yPos - valueBounds.size.y / 2.0f - 5.0f});

        renderTarget.draw(valueText);
    }

    // create labeles for horizontal axi
    for (size_t i = 0; i < xTickCount; i += xTickStep)
	{
        if (!drawXAxisLabels) 
            break;

        if (xTickCount - 1 - i < xTickStep)
			i = xTickCount - 1;

        const float xPos = signalContext.topLeft.x + (1.0f * static_cast<float>(i) * xSize / static_cast<float>(xTickCount - 1));

        sf::Text domainText(renderFont);
        domainText.setFillColor(axisColor);
        domainText.setCharacterSize(16);
        std::ostringstream domainStr;

        auto label = labels[i + xTickOffset];
        if (label.supportsInterface(IFloat::Id))
            domainStr << std::fixed << std::showpoint << std::setprecision(2) << static_cast<Float>(label);
        else if (label.supportsInterface(IInteger::Id))
            domainStr << static_cast<Int>(label);
        else
            domainStr << label;
            
        domainText.setString(domainStr.str());
        const auto domainBounds = domainText.getGlobalBounds();

        float xOffset;
        if (i == 0)
            xOffset = 0;
        else if (i == xTickCount - 1)
            xOffset = domainBounds.size.x;
        else
            xOffset = domainBounds.size.x / 2.0f;

        domainText.setPosition({xPos - xOffset, signalContext.bottomRight.y + domainBounds.size.y - 5.0f});

        renderTarget.draw(domainText);
    }

    if (drawTitle)
    {
        sf::Text signalText(renderFont);
        signalText.setStyle(sf::Text::Style::Bold);
        signalText.setFillColor(signalContext.getColor());
        signalText.setCharacterSize(16);
        signalText.setString(signalContext.caption);
        const auto valueBounds = signalText.getGlobalBounds();
        const auto d = signalText.getCharacterSize();
        signalText.setPosition(
            {(signalContext.topLeft.x + signalContext.bottomRight.x - valueBounds.size.x) / 2, signalContext.topLeft.y - d * 1.5f});

        renderTarget.draw(signalText);
    }
}

void RendererFbImpl::renderMultiTitle(sf::RenderTarget& renderTarget, const sf::Font& renderFont)
{
    std::vector<std::pair<sf::Text, sf::FloatRect>> list;
    float totalWidth = 0.0;
    for (auto sigIt = signalContexts.begin(); sigIt != signalContexts.end() - 1; ++sigIt)
    {
        sf::Text signalText(renderFont);
        signalText.setStyle(sf::Text::Style::Bold);
        signalText.setFillColor(sigIt->getColor());
        signalText.setCharacterSize(16);
        signalText.setString(sigIt->caption + " ");

        sf::FloatRect valueBounds = signalText.getGlobalBounds();
        totalWidth += valueBounds.size.x;

        list.push_back({std::move(signalText), std::move(valueBounds)});
    }

    float xPos = (bottomRight.x - totalWidth) / 2.0;
    for (auto& [text, bounds]: list)
    {
        text.setPosition({xPos, topLeft.y});
        xPos += bounds.size.x;
        renderTarget.draw(text);
    }
}

void RendererFbImpl::processAttributeChanged(SignalContext& signalContext,
                                             const StringPtr& attrName,
                                             const BaseObjectPtr& attrValue)
{
    if (attrName == "Name")
        signalContext.setCaption(attrValue);
}

void RendererFbImpl::subscribeToSignalCoreEvent(const SignalPtr& signal)
{
    signal.getOnComponentCoreEvent() += event(&RendererFbImpl::processCoreEvent);
}

void RendererFbImpl::processCoreEvent(ComponentPtr& component, CoreEventArgsPtr& args)
{
    if (args.getEventId() == static_cast<Int>(CoreEventId::AttributeChanged))
    {
        for (auto& sigCtx: signalContexts)
        {
            if (sigCtx.inputPort.getSignal() == component)
            {
                const auto params = args.getParameters();
                const auto name = params.get("AttributeName");
                const auto value = params.get(name);

                processAttributeChanged(sigCtx, name, value);
                break;
            }
        }
    }
}

}

END_NAMESPACE_REF_FB_MODULE
