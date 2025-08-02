#include <ref_fb_module/video_player_fb_impl.h>
#include <ref_fb_module/arial.ttf.h>
#include <opendaq/work_factory.h>
#include <opendaq/event_packet_params.h>
#include <coreobjects/callable_info_factory.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace VideoPlayer
{

VideoPlayerFbImpl::VideoPlayerFbImpl(const ContextPtr& ctx, 
                                     const ComponentPtr& parent, 
                                     const StringPtr& localId,
                                     const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
    , texture()
    , sprite(texture)
    , font(ARIAL_TTF, sizeof(ARIAL_TTF))
    , timestampText(font)
{
    if (!context.getScheduler().isMainLoopSet())
       DAQ_THROW_EXCEPTION(InvalidStateException, "Main loop is not set in the scheduler. Video player requires main loop for rendering.");

    initProperties();
    initInputPorts();

    timestampText.setCharacterSize(24);
    timestampText.setFillColor(sf::Color::White);
    timestampText.setOutlineColor(sf::Color::Black);
    timestampText.setOutlineThickness(2);
}

FunctionBlockTypePtr VideoPlayerFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleVideoPlayer",
                             "Video Player",
                             "Video playback and visualization"
    );
}

void VideoPlayerFbImpl::initProperties()
{
    objPtr.addProperty(FunctionPropertyBuilder("OpenWindow", ProcedureInfo()).setReadOnly(true).build());
    objPtr.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("OpenWindow", Procedure([this]
    {
        startRender();
    }));
}

void VideoPlayerFbImpl::initInputPorts()
{
    videoInputPort = createAndAddInputPort("VideoIp", PacketReadyNotification::Scheduler);
    videoInputPort.setActive(false);
}

void VideoPlayerFbImpl::updateTimestamp(const DataPacketPtr& domainPacket)
{
    bool timestampAvailable = false;
    if (domainPacket.assigned() && domainPacket.getSampleCount())
    {
        std::chrono::system_clock::time_point timePoint;
        if (timeReader.transform(domainPacket.getData(), &timePoint, 1, domainPacket.getDataDescriptor()))
        {
            std::time_t t = std::chrono::system_clock::to_time_t(timePoint);
            std::tm tm = *std::localtime(&t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            std::string timeStr = oss.str();

            timestampText.setString(timeStr);
            return;
        }
    }
    timestampText.setString("No timestamp available");
}

bool VideoPlayerFbImpl::closeWindow()
{
    window->close();
    dataPackets.clear();
    videoInputPort.setActive(false);
    return false;
}

void VideoPlayerFbImpl::startRender()
{
    if (window && window->isOpen())
        return;
    
    window.reset();
    videoInputPort.setActive(true);
    
    auto scheduler = context.getScheduler();
    auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();

    scheduler.scheduleWorkOnMainLoop(WorkRepetitive([this, thisWeakRef = thisWeakRef]
    {
        const auto thisFb = thisWeakRef.getRef();
        if (!thisFb.assigned())
            return false;

        if (window)
        {
            if (!window->isOpen())
                return false;

            while (auto event = window->pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                    return closeWindow();
            }
        }

        DataPacketPtr packet;
        if (!dataPackets.tryPopFront(packet))
            return true;

        const SizeT pictureSize = packet.getRawDataSize();
        const uint8_t* pictureData = static_cast<const uint8_t*>(packet.getData());

        if (!texture.loadFromMemory(pictureData, pictureSize))
        {
            closeWindow();
            DAQ_THROW_EXCEPTION(InvalidOperationException, "Failed to load image from memory");
        }

        if (!window)
            window = std::make_unique<sf::RenderWindow>(sf::VideoMode(texture.getSize()), "Video Playback");

        updateTimestamp(packet.getDomainPacket());
        timestampText.setPosition({5.0f,5.0f});

        sprite.setTexture(texture, true);
        
        window->clear(sf::Color::Black);
        window->draw(sprite);
        window->draw(timestampText);
        window->display();

        return true;
    }));
}

void VideoPlayerFbImpl::onPacketReceived(const InputPortPtr& port)
{
    auto lock = this->getAcquisitionLock();
    auto packets = port.getConnection().dequeueAll();
    for (const auto& packet : packets)
    {
        switch (packet.getType())
        {
            case PacketType::None:
                LOG_W("Packet type None");
                break;
            case PacketType::Data:
                handleDataPacket(packet.asPtr<IDataPacket>(true));
                break;
            case PacketType::Event:
                handleEventPacket(packet.asPtr<IEventPacket>(true));
                break;
        }
    }
}

void VideoPlayerFbImpl::handleDataPacket(const DataPacketPtr& packet)
{
    dataPackets.pushBack(DataPacketPtr(packet));
}

void VideoPlayerFbImpl::handleEventPacket(const EventPacketPtr& packet)
{
    auto params = packet.getParameters();
    if (!params.hasKey(event_packet_param::DATA_DESCRIPTOR))
        return;

    auto descriptor = params.get(event_packet_param::DATA_DESCRIPTOR).asPtr<IDataDescriptor>();

    if (descriptor.getSampleType() != SampleType::Binary)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Video player requires binary data descriptor");

    // auto unit = descriptor.getUnit();
    // if (unit.getSymbol() != "JPEG")
    //     DAQ_THROW_EXCEPTION(InvalidParameterException, "Video player requires JPEG data descriptor");

    auto scheduler = context.getScheduler();
    auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();

    scheduler.scheduleWorkOnMainLoop(Work([this, thisWeakRef = std::move(thisWeakRef)]
    {   
        const auto thisFb = thisWeakRef.getRef();
        if (!thisFb.assigned())
            return;

        if (!window)
            startRender();
    }));
}


} // namespace VideoPlayer
END_NAMESPACE_REF_FB_MODULE
