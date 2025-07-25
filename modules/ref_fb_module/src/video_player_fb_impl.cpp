#include <ref_fb_module/video_player_fb_impl.h>
#include <opendaq/work_factory.h>
#include <opendaq/event_packet_params.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace VideoPlayer
{

VideoPlayerFbImpl::VideoPlayerFbImpl(const ContextPtr& ctx, 
                                     const ComponentPtr& parent, 
                                     const StringPtr& localId,
                                     const PropertyObjectPtr& config)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    if (!context.getScheduler().isMainLoopSet())
       DAQ_THROW_EXCEPTION(InvalidStateException, "Main loop is not set in the scheduler. Video player requires main loop for rendering.");

    initInputPorts();
}

FunctionBlockTypePtr VideoPlayerFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleMjpegPlayer",
                             "Mjpeg Player",
                             "Mjpeg playback and visualization"
    );
}

void VideoPlayerFbImpl::initInputPorts()
{
    createAndAddInputPort("VideoIp", PacketReadyNotification::Scheduler);
}

void VideoPlayerFbImpl::onDisconnected(const InputPortPtr& port)
{

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
    auto scheduler = context.getScheduler();
    auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();

    scheduler.scheduleWorkOnMainLoop(Work([this, 
                                           thisWeakRef = thisWeakRef,
                                           packet = packet]
    {
        const auto thisFb = thisWeakRef.getRef();
        if (!thisFb.assigned() || !window)
            return;

        while (auto event = window->pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.reset();
                return;
            }
        }

        const SizeT pictureSize = packet.getRawDataSize();
        const uint8_t* jpegData = static_cast<const uint8_t*>(packet.getData());

        sf::Image image;
        if (!image.loadFromMemory(jpegData, pictureSize))
            DAQ_THROW_EXCEPTION(InvalidOperationException, "Failed to load image from memory");

        sf::Texture texture(image);
        sf::Sprite sprite(texture);

        window->clear();
        window->draw(sprite);
        window->display();
    }));
}

void VideoPlayerFbImpl::handleEventPacket(const EventPacketPtr& packet)
{
    auto params = packet.getParameters();
    if (!params.hasKey(event_packet_param::DATA_DESCRIPTOR))
        return;

    auto descriptor = params.get(event_packet_param::DATA_DESCRIPTOR).asPtr<IDataDescriptor>();

    if (descriptor.getSampleType() != SampleType::Binary)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Video player requires binary data descriptor");

    auto unit = descriptor.getUnit();
    if (unit.getSymbol() != "JPEG")
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Video player requires JPEG data descriptor");

    auto metadata = descriptor.getMetadata();
    if (!metadata.hasKey("FrameWidth") || !metadata.hasKey("FrameHeight"))
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Video player requires frameWidth and frameHeight in metadata");

    SizeT width = static_cast<SizeT>(std::stoul(static_cast<std::string>(metadata.get("FrameWidth"))));
    SizeT height = static_cast<SizeT>(std::stoul(static_cast<std::string>(metadata.get("FrameHeight"))));

    auto scheduler = context.getScheduler();
    auto thisWeakRef = this->template getWeakRefInternal<IFunctionBlock>();

    scheduler.scheduleWorkOnMainLoop(Work([this, 
                                          thisWeakRef = std::move(thisWeakRef),
                                          width,
                                          height]
    {
        if (width == frameWidth && height == frameHeight)
            return;
        
        frameWidth = width;
        frameHeight = height;
        
        const auto thisFb = thisWeakRef.getRef();
        if (!thisFb.assigned())
            return;

        if (!window)
            window = std::make_unique<sf::RenderWindow>(sf::VideoMode(sf::Vector2u(frameWidth, frameHeight)), "MJPEG Video Playback", sf::Style::Close | sf::Style::Titlebar);
    }));
}


} // namespace VideoPlayer
END_NAMESPACE_REF_FB_MODULE
