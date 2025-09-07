/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <ref_fb_module/common.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/function_block_type_factory.h>
#include <opendaq/sample_type.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/sample_type_traits.h>
#include <ref_fb_module/polyline.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/reader_utils.h>

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4242)
#endif

#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

#include <chrono>
#include <thread>
#include <condition_variable>
#include <queue>
#include <variant>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Renderer
{

template <SampleType ST>
struct DomainTypeCast
{
    static constexpr SampleType DomainSampleType = SampleType::Undefined;
};

#define DOMAIN_SAMPLE_TYPE(Source, Dest) \
    template <> \
    struct DomainTypeCast<Source> \
    { \
        static constexpr SampleType DomainSampleType = Dest; \
    };

DOMAIN_SAMPLE_TYPE(SampleType::Int8, SampleType::Int64)
DOMAIN_SAMPLE_TYPE(SampleType::Int16, SampleType::Int64)
DOMAIN_SAMPLE_TYPE(SampleType::Int32, SampleType::Int64)
DOMAIN_SAMPLE_TYPE(SampleType::Int64, SampleType::Int64)

DOMAIN_SAMPLE_TYPE(SampleType::UInt8, SampleType::UInt64)
DOMAIN_SAMPLE_TYPE(SampleType::UInt16, SampleType::UInt64)
DOMAIN_SAMPLE_TYPE(SampleType::UInt32, SampleType::UInt64)
DOMAIN_SAMPLE_TYPE(SampleType::UInt64, SampleType::UInt64)

DOMAIN_SAMPLE_TYPE(SampleType::Float32, SampleType::Float64)
DOMAIN_SAMPLE_TYPE(SampleType::Float64, SampleType::Float64)

using DomainStamp = std::variant<int64_t, uint64_t, double>;

struct GeneralContext
{
    bool hasTimeOrigin;
    double duration;
    bool freezed;
};

struct SignalContext
{
    size_t index;
    InputPortConfigPtr inputPort;
    GeneralContext* ctx;
    std::deque<DataPacketPtr> dataPackets;
    std::deque<DataPacketPtr> dataPacketsInFreezeMode;

    bool valid{ false };
    double max{ 0 };
    double min{ 0 };
    int64_t durationInTicks;
    bool isExplicit;
    bool isRange;
    SampleType sampleType;
    DataDescriptorPtr inputDataSignalDescriptor;
    DataDescriptorPtr inputDomainDataDescriptor;
    Int domainDelta;
    Int domainStart;
    Int domainResNum;
    Int domainResDen;
    Int domainToTimeNum;
    Int domainToTimeDen;
    std::string domainUnit;
    std::string domainQuantity;
    std::string caption;
    std::chrono::system_clock::time_point origin;
    bool hasTimeOrigin{false};

    SampleType domainSampleType;
    DomainStamp lastDomainStamp;
    DomainStamp firstDomainStamp;
    DomainStamp singleAxisLastDomainStamp;
    DomainStamp singleAxisFirstDomainStamp;

    std::chrono::system_clock::time_point lastTimeValue;
    std::chrono::system_clock::time_point firstTimeValue;

    sf::Vector2f topLeft;
    sf::Vector2f bottomRight;

    bool lastValueSet;
    Float lastValue;

    void processPacket();
    void processEventPacket(const EventPacketPtr& packet);
    void processDataPacket(const DataPacketPtr& packet);

    void setEpoch(std::string timeStr);
    sf::Color getColor() const;
    void setCaption(const std::string& caption = std::string {});

    void handleDataDescriptorChanged(const DataDescriptorPtr& descriptor);
    void handleDomainDescriptorChanged(const DataDescriptorPtr& descriptor);

    bool getValid();

    template <SampleType DST>
    void setLastDomainStamp(const DataPacketPtr& domainPacket);

    template <SampleType DomainSampleType>
    void timestampToTimePoint(typename SampleTypeToType<DomainSampleType>::Type timeStamp);

    template <SampleType DST>
    void setSingleXAxis(std::chrono::system_clock::time_point lastTimeValue, Float lastDomainValue);
};

class RendererFbImpl final : public FunctionBlock
{
public:
    explicit RendererFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config);
    ~RendererFbImpl() override;

    void onConnected(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;

    static FunctionBlockTypePtr CreateType();

protected:
    void removed() override;

private:
    bool rendererUsesMainLoop {false};
    std::unique_ptr<sf::RenderWindow> window;
    std::unique_ptr<sf::Font> renderFont;
    std::chrono::milliseconds defaultWaitTime;
    std::chrono::steady_clock::time_point waitTime;

    GeneralContext ctx;

    std::vector<SignalContext> signalContexts;
    std::thread renderThread;
    std::condition_variable cv;
    std::atomic<bool> rendererStopRequested;
    bool resolutionChangedFlag;
    float lineThickness;
    bool singleXAxis;
    bool singleYAxis;
    int resolution;
    bool showLastValue;
    bool useCustomMinMaxValue;
    double customMinValue;
    double customMaxValue;
    double yMaxValue;
    double yMinValue;
    bool useCustom2dRangeValue;
    size_t custom2dMinRange;
    size_t custom2dMaxRange;
    sf::Vector2f topLeft;
    sf::Vector2f bottomRight;
    size_t signalContextIndex;
    int inputPortCount;

    double lastDomainValue;
    std::chrono::system_clock::time_point lastTimeValue;

    std::string domainUnit;
    std::string domainQuantity;

    bool singleXAxisConfigured;

    sf::Color axisColor;

    ComponentStatus futureComponentStatus;
    StringPtr futureComponentMessage;

    void logAndSetFutureComponentStatus(ComponentStatus status, StringPtr message);

    void updateInputPorts();
    void renderSignals(sf::RenderTarget& renderTarget, const sf::Font& renderFont);
    void getWidthAndHeight(unsigned int& width, unsigned int& height);

    template <SampleType DST>
    void renderSignal(SignalContext& signalContext, sf::RenderTarget& renderTarget, const sf::Font& renderFont);

    template <SampleType DST>
    void renderPacket(SignalContext& signalContext,
                      sf::RenderTarget& renderTarget,
                      const sf::Font& renderFont,
                      const DataPacketPtr& packet,
                      bool& havePrevPacket,
                      typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type& nextExpectedDomainPacketValue,
                      std::unique_ptr<Polyline>& line,
                      bool& end);

    template <SampleType DST>
    void renderPacketImplicitAndExplicit(
        SignalContext& signalContext,
        DataRuleType domainRuleType,
        sf::RenderTarget& renderTarget,
        const  sf::Font& renderFont,
        const DataPacketPtr& packet,
        bool& havePrevPacket,
        typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type& nextExpectedDomainPacketValue,
        std::unique_ptr<Polyline>& line,
        bool& end);

    template <SampleType DST>
    void renderArrayPacketImplicitAndExplicit(
        SignalContext& signalContext,
        DataRuleType domainRuleType,
        sf::RenderTarget& renderTarget,
        const  sf::Font& renderFont,
        const DataPacketPtr& packet,
        bool& havePrevPacket,
        typename SampleTypeToType<DomainTypeCast<DST>::DomainSampleType>::Type& nextExpectedDomainPacketValue,
        std::unique_ptr<Polyline>& line,
        bool& end);

    void renderLoop();
    void processSignalContexts();

    void prepareSingleXAxis();
    void getYMinMax(const SignalContext& signalContext, double& yMax, double& yMin);

    void renderAxes(sf::RenderTarget& renderTarget, const sf::Font& renderFont);
    void renderAxis(sf::RenderTarget& renderTarget, SignalContext& signalContext, const sf::Font& renderFont, bool drawXAxisLabels, bool drawTitle);
    void renderMultiTitle(sf::RenderTarget& renderTarget, const sf::Font& renderFont);
    void processAttributeChanged(SignalContext& signalContext, const StringPtr& attrName, const BaseObjectPtr& attrValue);
    void subscribeToSignalCoreEvent(const SignalPtr& signal);
    void processCoreEvent(ComponentPtr& component, CoreEventArgsPtr& args);

    void resize(sf::RenderWindow& window);
    void initProperties();
    void propertyChanged();
    void resolutionChanged();
    void readProperties();
    void readResolutionProperty();

    template <SampleType DST>
    void domainStampToDomainValue(Float& lastDomainValue, const SignalContext& signalContext, DomainStamp domainStamp);

    template <typename Iter, typename Cont>
    bool isLastIter(Iter iter, const Cont& cont);
    void rendererStopRequesteding();

    void updateSingleXAxis();

    void startRendererWindow();
    void initializeRenderer();
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY, RendererFb, IFunctionBlock,
    IContext*, context,
    IString*, parentGlobalId,
    IString*, localId,
    IPropertyObject*, config
    );

}

END_NAMESPACE_REF_FB_MODULE
