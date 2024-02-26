#include <opendaq/function_block_ptr.h>
#include <ref_fb_module/arial.ttf.h>
#include <ref_fb_module/renderer_fb_impl.h>

#include <opendaq/event_packet_ids.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/custom_log.h>
#include <ref_fb_module/dispatch.h>
#include <coreobjects/eval_value_factory.h>

#include <date/date.h>

#include <iomanip>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Renderer
{

RendererFbImpl::RendererFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , stopRender(false)
    , resChanged(false)
    , signalContextIndex(0)
    , inputPortCount(0)
    , axisColor(150, 150, 150)
{
    initProperties();
    updateInputPorts();

    LOGP_D("Starting render thread")
    renderThread = std::thread{ &RendererFbImpl::renderLoop, this };
}

void RendererFbImpl::stopRendering()
{
    if (!stopRender)
    {
        stopRender = true;
        cv.notify_one();
    }
}

RendererFbImpl::~RendererFbImpl()
{
    stopRendering();

    renderThread.join();
    LOGP_D("Render thread stopped")
}

FunctionBlockTypePtr RendererFbImpl::CreateType()
{
    return FunctionBlockType(
        "ref_fb_module_renderer",
        "Renderer",
        "Signal visualization"
    );
}

void RendererFbImpl::removed()
{
    stopRendering();

    FunctionBlockImpl::removed();
}

void RendererFbImpl::initProperties()
{
    const auto durationProp = FloatPropertyBuilder("Duration", 1.0).setSuggestedValues(List<Float>(0.01, 0.1, 1.0, 10.0)).build();

    objPtr.addProperty(durationProp);
    objPtr.getOnPropertyValueWrite("Duration") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
        propertyChanged();
    };

    const auto singleXAxisProp = BoolProperty("SingleXAxis", False);
    objPtr.addProperty(singleXAxisProp);
    objPtr.getOnPropertyValueWrite("SingleXAxis") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
        propertyChanged();
    };

    const auto singleYAxisProp = BoolPropertyBuilder("SingleYAxis", False).setVisible(EvalValue("$SingleXAxis")).build();
    objPtr.addProperty(singleYAxisProp);
    objPtr.getOnPropertyValueWrite("SingleYAxis") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    const auto lineThicknessProp = FloatProperty("LineThickness", 1.0);
    objPtr.addProperty(lineThicknessProp);
    objPtr.getOnPropertyValueWrite("LineThickness") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    const auto freezeProp = BoolProperty("Freeze", False);
    objPtr.addProperty(freezeProp);
    objPtr.getOnPropertyValueWrite("Freeze") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
        propertyChanged();
    };

    const auto resolutionProp = SelectionProperty("Resolution", List<IString>("640x480", "800x600", "1024x768", "1280x720"), 1);
    objPtr.addProperty(resolutionProp);
    objPtr.getOnPropertyValueWrite("Resolution") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
        resolutionChanged();
    };

    const auto showLastValueProp = BoolProperty("ShowLastValue", False);
    objPtr.addProperty(showLastValueProp);
    objPtr.getOnPropertyValueWrite("ShowLastValue") += [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args)
    {
        propertyChanged();
    };


    const auto useCustomMinMaxValue = BoolProperty("UseCustomMinMaxValue", False);
    objPtr.addProperty(useCustomMinMaxValue);
    objPtr.getOnPropertyValueWrite("UseCustomMinMaxValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    const auto customHighValueProp = FloatPropertyBuilder("CustomMaxValue", 10.0).setVisible(EvalValue("$UseCustomMinMaxValue")).build();
    objPtr.addProperty(customHighValueProp);
    objPtr.getOnPropertyValueWrite("CustomMaxValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    const auto customLowValueProp = FloatPropertyBuilder("CustomMinValue", -10.0).setVisible(EvalValue("$UseCustomMinMaxValue")).build();
    objPtr.addProperty(customLowValueProp);
    objPtr.getOnPropertyValueWrite("CustomMinValue") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(); };

    readProperties();
    readResolutionProperty();
}

void RendererFbImpl::propertyChanged()
{
    std::scoped_lock lock(sync);
    readProperties();
}

void RendererFbImpl::resolutionChanged()
{
    std::scoped_lock lock(sync);
    readResolutionProperty();
    resChanged = true;
}

void RendererFbImpl::readProperties()
{
    duration = objPtr.getPropertyValue("Duration");
    LOG_T("Properties: Duration {}", duration)
    singleXAxis = objPtr.getPropertyValue("SingleXAxis");
    LOG_T("Properties: SingleXAxis {}", singleXAxis)
    singleYAxis = objPtr.getPropertyValue("SingleYAxis");
    LOG_T("Properties: SingleYAxis {}", singleYAxis)
    freeze = objPtr.getPropertyValue("Freeze");
    LOG_T("Properties: Freeze {}", freeze)
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
}

void RendererFbImpl::readResolutionProperty()
{
    resolution = objPtr.getPropertyValue("Resolution");
    LOG_T("Properties: Resolution {}", resolution)
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
            ++it;
    }

    const auto inputPort = createAndAddInputPort(fmt::format("Input{}", inputPortCount++), PacketReadyNotification::SameThread);

    signalContexts.emplace_back(SignalContext{ 0, inputPort });
    for (size_t i = 0; i < signalContexts.size(); i++)
        signalContexts[i].index = i;
}

void RendererFbImpl::renderSignals(sf::RenderTarget& renderTarget, const sf::Font& font)
{
    for (auto& sigCtx : signalContexts)
    {
        if (sigCtx.valid)
            SAMPLE_TYPE_DISPATCH(sigCtx.domainSampleType, renderSignal, sigCtx, renderTarget, font);
    }
}

sf::Color RendererFbImpl::getColor(const SignalContext& signalContext)
{
    sf::Color color;
    switch(signalContext.index % 6)
    {
        case 0:
            color = sf::Color::Red;
            break;
        case 1:
            color = sf::Color::Yellow;
            break;
        case 2:
            color = sf::Color::Green;
            break;
        case 3:
            color = sf::Color::Magenta;
            break;
        case 4:
            color = sf::Color::Cyan;
            break;
        case 5:
            color = sf::Color::Blue;
            break;
        default:
            color = sf::Color::White;
    }
    return color;
}

/*
void RendererFbImpl::renderSignalExplicitRange(SignalContext& signalContext, sf::RenderTarget& renderTarget) const
{
    if (signalContext.dataPackets.empty())
        return;

    auto packetIt = signalContext.dataPackets.begin();
    auto domainPacket = packetIt->getDomainPacket();

    if (domainPacket.getSampleCount() == 0)
        return;

    const float xSize = signalContext.bottomRight.x - signalContext.topLeft.x;
    const float xOffset = signalContext.topLeft.x;
    const float ySize = signalContext.bottomRight.y - signalContext.topLeft.y;
    const float yOffset = signalContext.bottomRight.y;

    constexpr float lineThickness = 1.0f;
    const sf::Color color = getColor(signalContext);

    Polyline line(lineThickness, LineStyle::solid);
    line.setColor(color);

    const auto domainData_ = static_cast<RangeType<int64_t>*>(domainPacket.getData());
    auto domainValue = *(domainData_ + domainPacket.getSampleCount() - 1);
    const auto firstDomainValueLimit = domainValue.end - signalContext.durationInTicks;
    const auto domainRange = domainValue.end - firstDomainValueLimit;
    if (domainRange == 0)
        return;

    while (packetIt != signalContext.dataPackets.end())
    {
        domainPacket = packetIt->getDomainPacket();
        const auto samplesInPacket = packetIt->getSampleCount();
        auto data = static_cast<double*>(packetIt->getData()) + samplesInPacket;
        auto domainData = static_cast<RangeType<int64_t>*>(domainPacket.getData()) + samplesInPacket;

        for (size_t i = 0; i < samplesInPacket; i++)
        {
            const double value = *(--data);
            domainValue = *(--domainData);
            if (domainValue.start < firstDomainValueLimit)
                goto end;

            const float xPos1 = xOffset + static_cast<double>(domainValue.start - firstDomainValueLimit) / static_cast<double>(domainRange) * static_cast<float>(xSize);
            const float xPos2 = xOffset + static_cast<double>(domainValue.end - firstDomainValueLimit) / static_cast<double>(domainRange) * static_cast<float>(xSize);
            const float yPos = yOffset - ((value - signalContext.min) / (signalContext.max - signalContext.min) * static_cast<float>(ySize));
            line.addPoint(xPos1, yPos);
            line.addPoint(xPos2, yPos);
            line.reset();
        }

        ++packetIt;
    }

end:

    signalContext.dataPackets.erase(packetIt, signalContext.dataPackets.end());

    renderTarget.draw(line);
}
*/

template <SampleType DST>
void RendererFbImpl::renderPacket(
    SignalContext& signalContext,
    sf::RenderTarget& renderTarget,
    const sf::Font& font,
    const DataPacketPtr& packet,
    bool& havePrevPacket,
    typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type& nextExpectedDomainPacketValue,
    std::unique_ptr<Polyline>& line,
    bool& end)
{
    auto domainPacket = packet.getDomainPacket();
    if (domainPacket.getSampleCount() == 0)
        return;

    auto domainDataDescriptor = domainPacket.getDataDescriptor();
    auto domainRule = domainDataDescriptor.getRule();
    size_t signalDimension = signalContext.inputDataSignalDescriptor.getDimensions().getCount();
    
    if (signalDimension == 1)
        renderArrayPacketImplicitAndExplicit<DST>(signalContext, domainRule.getType(), renderTarget, font, packet, havePrevPacket, nextExpectedDomainPacketValue, line, end);
    else
        renderPacketImplicitAndExplicit<DST>(signalContext, domainRule.getType(), renderTarget, font, packet, havePrevPacket, nextExpectedDomainPacketValue, line, end);
}

template <SampleType DST>
void RendererFbImpl::renderPacketImplicitAndExplicit(
    SignalContext& signalContext,
    DataRuleType domainRuleType,
    sf::RenderTarget& renderTarget,
    const sf::Font& font,
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
        line->setColor(getColor(signalContext));
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
    const sf::Font& font,
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
    
    size_t count = signalContext.inputDataSignalDescriptor.getDimensions()[0].getSize();
    
    double domainFactor = static_cast<double>(count-1) / static_cast<double>(xSize);

    double yMax, yMin;
    getYMinMax(signalContext, yMax, yMin);

    auto valueFactor = (yMax - yMin) / static_cast<double>(ySize);

    end = true;
    havePrevPacket = false;
    auto data = reinterpret_cast<uint8_t*>(packet.getData());

    if (samplesInPacket == 0)
        return;

    double value;
    for (size_t idx = 0; idx < count; idx++) 
    {
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

        float xPos = xOffset + static_cast<float>(1.0 * idx / domainFactor);
        float yPos = yOffset - static_cast<float>((value - yMin) / valueFactor);

        if (yPos < signalContext.topLeft.y)
            yPos = signalContext.topLeft.y;
        else if (yPos > signalContext.bottomRight.y)
            yPos = signalContext.bottomRight.y;

        line->addPoint(xPos, yPos);
    }
}

template <SampleType DST>
void RendererFbImpl::renderSignal(SignalContext& signalContext, sf::RenderTarget& renderTarget, const sf::Font& font)
{
    signalContext.lastValueSet = false;
    if (signalContext.dataPackets.empty())
        return;

    const sf::Color color = getColor(signalContext);

    auto line = std::make_unique<Polyline>(lineThickness, LineStyle::solid);
    line->setColor(color);

    typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type nextExpectedDomainPacketValue{};
    bool havePrevPacket = false;
    bool end = false;

    auto packetIt = signalContext.dataPackets.begin();
    for (; packetIt != signalContext.dataPackets.end(); ++packetIt)
    {
        renderPacket<DST>(signalContext, renderTarget, font, *packetIt, havePrevPacket, nextExpectedDomainPacketValue, line, end);

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
        sf::Text lastValueText;
        lastValueText.setFont(font);
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
        float top = signalContext.topLeft.y - valueBounds.height * 2.0f;
        if (singleXAxis && singleYAxis)
            top += signalContext.index * valueBounds.height * 2.0f;

        lastValueText.setPosition({signalContext.bottomRight.x - valueBounds.width, top});

        renderTarget.draw(lastValueText);
    }
}

void RendererFbImpl::onConnected(const InputPortPtr& inputPort)
{
    std::scoped_lock lock(sync);

    subscribeToSignalCoreEvent(inputPort.getSignal());
    updateInputPorts();
    LOG_T("Connected to port {}", inputPort.getLocalId());
}

void RendererFbImpl::onDisconnected(const InputPortPtr& inputPort)
{
    std::scoped_lock lock(sync);

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
                LOG_W("Renderer has multiple input signals with different dimension. Property singleXAxis has turned off");
                break;
            }
        }
    }
}

void RendererFbImpl::renderLoop()
{
    unsigned int width;
    unsigned int height;
    getWidthAndHeight(width, height);
    sf::RenderWindow window{sf::VideoMode({width, height}), "Renderer", sf::Style::Close | sf::Style::Titlebar};
    topLeft = sf::Vector2(0.0f, 0.0f);
    bottomRight = sf::Vector2f(width, height);

    sf::Font font;
    if (!font.loadFromMemory(ARIAL_TTF, sizeof(ARIAL_TTF)))
        throw std::runtime_error("Failed to load font.");

    std::unique_lock<std::mutex> lock(sync);
    const auto defaultWaitTime = std::chrono::milliseconds(20);
    auto waitTime = defaultWaitTime;
    while (!stopRender && window.isOpen())
    {
        cv.wait_for(lock, waitTime);
        auto t1 = std::chrono::steady_clock::now();
        if (!stopRender && window.isOpen())
        {
            if (resChanged)
            {
                resChanged = false;
                resize(window);
            }

            sf::Event event{};
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.close();
            }

            processSignalContexts();

            window.clear();

            updateSingleXAxis();

            if (singleXAxis)
                prepareSingleXAxis();
            renderAxes(window, font);
            renderSignals(window, font);

            window.display();
        }
        auto t2 = std::chrono::steady_clock::now();

        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
        waitTime = defaultWaitTime - duration;
        if (waitTime < std::chrono::milliseconds(1))
            waitTime = std::chrono::milliseconds(1);
    }

    resChanged = false;
}

void RendererFbImpl::resize(sf::RenderWindow& window)
{
    unsigned int width;
    unsigned int height;
    getWidthAndHeight(width, height);
    topLeft = sf::Vector2(0.0f, 0.0f);
    bottomRight = sf::Vector2f(width, height);

    for (auto sigCtx : signalContexts)
    {
        sigCtx.topLeft = topLeft;
        sigCtx.bottomRight = bottomRight;
    }

    window.setSize(sf::Vector2u(width, height));
    sf::FloatRect viewRect(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    window.setView(sf::View(viewRect));
}

void RendererFbImpl::processSignalContexts()
{
    for (auto& sigCtx: signalContexts)
        processSignalContext(sigCtx);
}

template <typename Iter, typename Cont>
bool RendererFbImpl::isLastIter(Iter iter, const Cont& cont)
{
    return (iter != cont.end()) && (std::next(iter) != cont.end()) &&  (std::next(std::next(iter)) == cont.end());
}

void RendererFbImpl::renderAxes(sf::RenderTarget& renderTarget, const sf::Font& font)
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
                renderAxis(renderTarget, *sigIt, font, true, false);
        }
        renderMultiTitle(renderTarget, font);
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
            renderAxis(renderTarget, *sigIt, font, drawXAxisLabels, true);
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
            throw InvalidStateException("First signal not valid");

        hasTimeOrigin = sigIt->hasTimeOrigin;
        SAMPLE_TYPE_DISPATCH(sigIt->domainSampleType, domainStampToDomainValue, lastDomainValue, *sigIt, sigIt->lastDomainStamp)
        lastTimeValue = sigIt->lastTimeValue;
        domainUnit = sigIt->domainUnit;
        domainQuantity = sigIt->domainQuantity;

        for (; sigIt != signalContexts.end() - 1; ++sigIt)
        {
            if (!sigIt->valid)
                continue;

            if (sigIt->hasTimeOrigin != hasTimeOrigin)
            {
               throw InvalidStateException("time origin set on some signals but not all");
               return;
            }

            if (!hasTimeOrigin)
            {
               if (domainUnit != sigIt->domainUnit)
               {
                   throw InvalidStateException("domain unit not equal");
                   return;
               }

               if (domainQuantity != sigIt->domainQuantity)
               {
                   throw InvalidStateException("domain quantity not equal");
                   return;
               }
            }

            if (hasTimeOrigin)
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
            SAMPLE_TYPE_DISPATCH(sigIt->domainSampleType, setSingleXAxis, *sigIt, lastTimeValue, lastDomainValue)
        }

    }
    catch (const std::exception& e)
    {
        LOG_W("Unable to configure single X axis: {}", e.what());
    }
}

template <SampleType DST>
void RendererFbImpl::setSingleXAxis(SignalContext& signalContext, std::chrono::system_clock::time_point lastTimeValue, Float lastDomainValue)
{
    using DestDomainType = typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type;

    if (hasTimeOrigin)
    {
        auto firstTimeValue = lastTimeValue - timeValueToDuration(signalContext, duration);
        signalContext.singleAxisLastDomainStamp = static_cast<DestDomainType>(
            lastTimeValue.time_since_epoch().count() * signalContext.domainToTimeDen / signalContext.domainToTimeNum);
        signalContext.singleAxisFirstDomainStamp = static_cast<DestDomainType>(
            firstTimeValue.time_since_epoch().count() * signalContext.domainToTimeDen / signalContext.domainToTimeNum);
    }
    else
    {
        auto firstDomainValue = lastDomainValue - duration;
        signalContext.singleAxisLastDomainStamp =
            static_cast<DestDomainType>(lastDomainValue * signalContext.domainResDen / signalContext.domainResNum);
        signalContext.singleAxisFirstDomainStamp =
            static_cast<DestDomainType>(firstDomainValue * signalContext.domainResDen / signalContext.domainResNum);
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


void RendererFbImpl::renderAxis(sf::RenderTarget& renderTarget, SignalContext& signalContext, const sf::Font& font, bool drawXAxisLabels, bool drawTitle)
{
    size_t xTickCount = 5;
    size_t yTickCount = 5;

    daq::ListPtr<daq::IBaseObject> labels{};

    size_t signalDimension = signalContext.inputDataSignalDescriptor.getDimensions().getCount();

    if (signalDimension == 1) 
    {
        auto domainDataDimension = signalContext.inputDataSignalDescriptor.getDimensions()[0];
        labels = domainDataDimension.getLabels();
        xTickCount = labels.getCount();
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

    // create vertical grid
    for (size_t i = 1; i < xTickCount; i++)
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

        sf::Text valueText;
        valueText.setFont(font);
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
        valueText.setPosition({signalContext.topLeft.x - valueBounds.width - d, yPos - valueBounds.height / 2.0f - 5.0f});

        renderTarget.draw(valueText);
    }
    
    // create labeles for horizontal axi
    for (size_t i = 0; i < xTickCount; i++)
    {
        if (!drawXAxisLabels) 
            break;

        const float xPos = signalContext.topLeft.x + (1.0f * static_cast<float>(i) * xSize / static_cast<float>(xTickCount - 1));

        // for absolute time show only 3 domain values, otherwise 5
        if (i % 2 == 0 || !signalContext.hasTimeOrigin)
        {
            sf::Text domainText;
            domainText.setFont(font);
            domainText.setFillColor(axisColor);
            domainText.setCharacterSize(16);
            std::ostringstream domainStr;
            if (signalDimension == 1) 
            {
                domainStr << std::fixed << std::showpoint << std::setprecision(2) << labels[i];
            }
            else if (signalContext.hasTimeOrigin)
            {
                const auto tp = signalContext.lastTimeValue - timeValueToDuration(signalContext, duration * (static_cast<double>(xTickCount - 1 - i) / static_cast<double>(xTickCount - 1)));
                const auto tpms = date::floor<std::chrono::milliseconds>(tp);
                domainStr << date::format("%F %T", tpms);
            }
            else
                domainStr << std::fixed << std::showpoint << std::setprecision(2) << duration * (static_cast<double>(i) / static_cast<double>(xTickCount - 1));

            domainText.setString(domainStr.str());
            const auto domainBounds = domainText.getGlobalBounds();
            float xOffset;
            if (i == 0)
                xOffset = 0;
            else if (i == xTickCount - 1)
                xOffset = domainBounds.width;
            else
                xOffset = domainBounds.width / 2.0f;

            domainText.setPosition({xPos - xOffset, signalContext.bottomRight.y + domainBounds.height - 5.0f});

            renderTarget.draw(domainText);
        }
    }

    if (drawTitle)
    {
        sf::Text signalText;
        signalText.setFont(font);
        signalText.setStyle(sf::Text::Style::Bold);
        signalText.setFillColor(getColor(signalContext));
        signalText.setCharacterSize(16);
        signalText.setString(signalContext.caption);
        const auto valueBounds = signalText.getGlobalBounds();
        const auto d = signalText.getCharacterSize();
        signalText.setPosition(
            {(signalContext.topLeft.x + signalContext.bottomRight.x - valueBounds.width) / 2, signalContext.topLeft.y - d * 1.5f});

        renderTarget.draw(signalText);
    }
}

void RendererFbImpl::renderMultiTitle(sf::RenderTarget& renderTarget, const sf::Font& font)
{
    std::vector<std::pair<std::shared_ptr<sf::Text>, sf::FloatRect>> list;
    float totalWidth = 0.0;
    for (auto sigIt = signalContexts.begin(); sigIt != signalContexts.end() - 1; ++sigIt)
    {
        std::shared_ptr<sf::Text> signalText = std::make_shared<sf::Text>();
        signalText->setFont(font);
        signalText->setStyle(sf::Text::Style::Bold);
        signalText->setFillColor(getColor(*sigIt));
        signalText->setCharacterSize(16);
        signalText->setString(sigIt->caption + " ");
        const auto valueBounds = signalText->getGlobalBounds();
        totalWidth += valueBounds.width;

        list.push_back({signalText, valueBounds});
    }

    float xPos = (bottomRight.x - totalWidth) / 2.0;
    for (auto& item: list)
    {
        item.first->setPosition({xPos, topLeft.y});

        xPos += item.second.width;
        renderTarget.draw(*(item.first));
    }
}


void RendererFbImpl::processSignalContext(SignalContext& signalContext)
{
    const auto conn = signalContext.inputPort.getConnection();
    if (!conn.assigned())
        return;

    PacketPtr packet = conn.dequeue();
    while (packet.assigned())
    {
        if (packet.supportsInterface<IEventPacket>())
        {
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            LOG_T("Processing {} event", eventPacket.getEventId())
            if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                DataDescriptorPtr valueSignalDescriptor = eventPacket.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
                DataDescriptorPtr domainSignalDescriptor = eventPacket.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
                processSignalDescriptorChanged(signalContext, valueSignalDescriptor, domainSignalDescriptor);
            }
        }
        else if (packet.getType() == PacketType::Data)
        {
            auto dataPacket = packet.asPtr<IDataPacket>();
            processDataPacket(signalContext, dataPacket);
        }

        packet = conn.dequeue();
    }
}

void RendererFbImpl::processSignalDescriptorChanged(SignalContext& signalContext, const DataDescriptorPtr& valueSignalDescriptor, const DataDescriptorPtr& domainSignalDescriptor)
{
    if (domainSignalDescriptor.assigned())
        signalContext.inputDomainDataDescriptor = domainSignalDescriptor;

    if (valueSignalDescriptor.assigned())
        signalContext.inputDataSignalDescriptor = valueSignalDescriptor;

    configureSignalContext(signalContext);
}

void RendererFbImpl::processAttributeChanged(SignalContext& signalContext,
                                             const StringPtr& attrName,
                                             const StringPtr& attrValue)
{
    if (attrName == "Name")
        setSignalContextCaption(signalContext, attrValue);
}

void RendererFbImpl::subscribeToSignalCoreEvent(const SignalPtr& signal)
{
    signal.getOnComponentCoreEvent() += event(&RendererFbImpl::processCoreEvent);
}

void RendererFbImpl::processCoreEvent(ComponentPtr& component, CoreEventArgsPtr& args)
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

void RendererFbImpl::configureSignalContext(SignalContext& signalContext)
{
    signalContext.valid = false;
    if (!signalContext.inputDataSignalDescriptor.assigned() || !signalContext.inputDomainDataDescriptor.assigned())
    {
        LOG_D("Incomplete input signal descriptors")
        return;
    }

    try
    {
        const auto domainDataDescriptor = signalContext.inputDomainDataDescriptor;
        if (!domainDataDescriptor.getTickResolution().assigned())
        {
            LOGP_W("Domain resolution not assigned")
            return;
        }

        auto domainRes = domainDataDescriptor.getTickResolution();
        if (domainRes.assigned())
        {
            signalContext.domainResNum = domainRes.getNumerator();
            signalContext.domainResDen = domainRes.getDenominator();
            Int gcd = std::gcd(signalContext.domainResDen, signalContext.domainResNum);
            signalContext.domainResNum /= gcd;
            signalContext.domainResDen /= gcd;
        }
        else
        {
            signalContext.domainResNum = 1;
            signalContext.domainResDen = 1;
        }

        const auto domainRule = domainDataDescriptor.getRule();
        if (domainRule.getType() == DataRuleType::Linear)
        {
            const auto domainRuleParams = domainRule.getParameters();
            try
            {
                signalContext.domainDelta = domainRuleParams.get("delta");
                signalContext.domainStart = domainRuleParams.get("start");
            }
            catch (const std::exception& e)
            {
                throw InvalidPropertyException("Invalid data rule parameters: {}", e.what());
            }
            signalContext.isExplicit = false;
        }
        else
        {
            signalContext.durationInTicks = static_cast<int64_t>(duration / static_cast<double>(domainDataDescriptor.getTickResolution()));
            signalContext.isExplicit = true;
            signalContext.isRange = domainDataDescriptor.getSampleType() == SampleType::RangeInt64;
        }

        signalContext.domainUnit = domainDataDescriptor.getUnit().getSymbol().toStdString();
        signalContext.domainQuantity = domainDataDescriptor.getUnit().getQuantity().toStdString();
        signalContext.domainSampleType = domainDataDescriptor.getSampleType();

        signalContext.hasTimeOrigin = false;
        const auto origin = domainDataDescriptor.getOrigin();
        if (origin.assigned() && !origin.toStdString().empty())
        {
            try
            {
                if (domainDataDescriptor.getUnit().getSymbol() != "s" || domainDataDescriptor.getUnit().getQuantity() != "time")
                {
                   LOGP_W("Domain signal not time, origin ignored")
                }
                else
                {
                   signalContext.origin = timeStrToTimePoint(origin);
                   signalContext.hasTimeOrigin = true;
                   LOG_D("Origin={}", date::format("%F %T", signalContext.origin))

                   int64_t n = signalContext.domainResNum * std::chrono::system_clock::time_point::period::den;
                   int64_t d = signalContext.domainResDen * std::chrono::system_clock::time_point::period::num;

                   int64_t gcd_n_d = std::gcd(n, d);
                   n /= gcd_n_d;
                   d /= gcd_n_d;

                   signalContext.domainToTimeNum = n;
                   signalContext.domainToTimeDen = d;
                }
            }
            catch (...)
            {
                LOG_W("Invalid origin, ignored: {}", origin)
            }
        }

        const auto dataDescriptor = signalContext.inputDataSignalDescriptor;
        if (dataDescriptor.getDimensions().getCount() > 1)  // matrix not supported on the input
        {
            LOG_W("Matrix signals not supported")
            return;
        }
        signalContext.sampleType = dataDescriptor.getSampleType();

        const auto valueRange = dataDescriptor.getValueRange();
        if (!valueRange.assigned())
        {
            signalContext.min = 0;
            signalContext.max = 1;
        }
        else
        {
            signalContext.min = dataDescriptor.getValueRange().getLowValue();
            signalContext.max = dataDescriptor.getValueRange().getHighValue();
        }
        setSignalContextCaption(signalContext);

        signalContext.valid = true;
        LOGP_D("Signal descriptor changed")
    }
    catch (const std::exception& e)
    {
        LOG_E("Signal descriptor changed error: {}", e.what())
    }
}

void RendererFbImpl::setSignalContextCaption(SignalContext& signalContext, const std::string& caption)
{
    if (caption.empty())
    {
        auto sig = signalContext.inputPort.getSignal();
        if (sig.assigned())
            signalContext.caption = static_cast<std::string>(sig.getName());
        else
            signalContext.caption = "N/A";
    }
    else
        signalContext.caption = caption;

    auto unit = signalContext.inputDataSignalDescriptor.getUnit();
    if (unit.assigned() && !unit.getSymbol().toStdString().empty())
        signalContext.caption += fmt::format(" [{}]", unit.getSymbol().toStdString());
}

void RendererFbImpl::processDataPacket(SignalContext& signalContext, const DataPacketPtr& dataPacket)
{
    if (!signalContext.valid)
        return;

    auto domainPacket = dataPacket.getDomainPacket();
    if (!domainPacket.assigned())
    {
        LOGP_W("Packet recieved, but no domain packet assigned. Packet ignored")
        return;
    }

    if (freeze)
    {
        signalContext.dataPacketsInFreezeMode.push_front(dataPacket);
        while (signalContext.dataPacketsInFreezeMode.size() > 1000)
            signalContext.dataPacketsInFreezeMode.pop_back();
    }
    else
    {
        while (!signalContext.dataPacketsInFreezeMode.empty())
        {
            auto pkt = signalContext.dataPacketsInFreezeMode.back();
            signalContext.dataPackets.push_front(pkt);
            signalContext.dataPacketsInFreezeMode.pop_back();
        }
        signalContext.dataPackets.push_front(dataPacket);
        SAMPLE_TYPE_DISPATCH(signalContext.domainSampleType, setLastDomainStamp, signalContext, domainPacket)
    }
}

template <SampleType DST>
void RendererFbImpl::setLastDomainStamp(SignalContext& signalContext, const DataPacketPtr& domainPacket)
{ 
    using SourceDomainType = typename SampleTypeToType<DST>::Type;
    using DestDomainType = typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type;

    const auto domainDataDescriptor = domainPacket.getDataDescriptor();

    DestDomainType lastDomainStamp;
    if (signalContext.isExplicit)
    {
        const auto domainDataPtr = static_cast<SourceDomainType*>(domainPacket.getData());
        lastDomainStamp = static_cast<DestDomainType>(*(domainDataPtr + domainPacket.getSampleCount() - 1));
    }
    else
    {
        NumberPtr offset = 0;
        if (domainPacket.getOffset().assigned())
            offset = domainPacket.getOffset();
        lastDomainStamp = static_cast<DestDomainType>(offset + domainPacket.getSampleCount() * signalContext.domainDelta +
                                                      signalContext.domainStart); 
    }

    signalContext.lastDomainStamp = lastDomainStamp;
    DestDomainType domainDuration = duration * signalContext.domainResDen / signalContext.domainResNum;
    signalContext.firstDomainStamp = lastDomainStamp - domainDuration;

    if (signalContext.hasTimeOrigin)
    {
        signalContext.lastTimeValue = timestampToTimePoint<DomainTypeCast<DST>::DomainSampleType>(signalContext, lastDomainStamp);
        signalContext.firstTimeValue = lastTimeValue - timeValueToDuration(signalContext, duration);
    }
}

std::string RendererFbImpl::fixUpIso8601(std::string epoch)
{
    if (epoch.find('T') == std::string::npos)
    {
        // If no time assume Midnight UTC
        epoch += "T00:00:00+00:00";
    }
    else if (epoch[epoch.size() - 1] == 'Z')
    {
        // If time-zone marked as "Zulu" (UTC) replace with offset
        epoch = epoch.erase(epoch.size() - 1) + "+00:00";
    }
    else if (epoch.find('+') == std::string::npos)
    {
        // If not time-zone offset assume UTC
        epoch += "+00:00";
    }

    return epoch;
}

std::chrono::system_clock::time_point RendererFbImpl::timeStrToTimePoint(std::string timeStr)
{
    std::istringstream timeStringStream(fixUpIso8601(timeStr));
    std::chrono::system_clock::time_point timePoint;
    timeStringStream >> date::parse("%FT%T%z", timePoint);
    if (timeStringStream.fail())
        throw std::runtime_error("Invalid format");

    return timePoint;
}

template <SampleType DomainSampleType>
std::chrono::system_clock::time_point RendererFbImpl::timestampToTimePoint(const SignalContext& signalContext, typename SampleTypeToType<DomainSampleType>::Type timeStamp)
{
    static_assert(DomainSampleType == SampleType::Float64 || DomainSampleType == SampleType::Int64 ||
                  DomainSampleType == SampleType::UInt64);

    if constexpr (std::is_floating_point_v<typename SampleTypeToType<DomainSampleType>::Type>)
    {
        const auto tp = signalContext.origin + timeValueToDuration(signalContext, timeStamp);
        return tp;
    }
    else
    {
        const auto tp = signalContext.origin + std::chrono::system_clock::time_point::duration(
            timeStamp * signalContext.domainToTimeNum / signalContext.domainToTimeDen);
        return tp;
    }
}

std::chrono::system_clock::duration RendererFbImpl::timeValueToDuration(const SignalContext& signalContext, Float timeValue)
{
    auto floatDuration = std::chrono::duration<float>(timeValue);
    auto dur = std::chrono::round<std::chrono::system_clock::duration>(floatDuration);
    return dur;
}

}

END_NAMESPACE_REF_FB_MODULE
