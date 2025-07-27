#include <ref_fb_module/video_player_fb_impl.h>
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
{
    if (!context.getScheduler().isMainLoopSet())
       DAQ_THROW_EXCEPTION(InvalidStateException, "Main loop is not set in the scheduler. Video player requires main loop for rendering.");

    initProperties();
    initInputPorts();
}

FunctionBlockTypePtr VideoPlayerFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleMjpegPlayer",
                             "Mjpeg Player",
                             "Mjpeg playback and visualization"
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
                {
                    window->close();
                    dataPackets.clear();
                    videoInputPort.setActive(false);
                    return false;
                }
            }
        }

        DataPacketPtr packet;
        if (!dataPackets.tryPopFront(packet))
            return true;

        const SizeT pictureSize = packet.getRawDataSize();
        const uint8_t* jpegData = static_cast<const uint8_t*>(packet.getData());

        if (!texture.loadFromMemory(jpegData, pictureSize))
            DAQ_THROW_EXCEPTION(InvalidOperationException, "Failed to load image from memory");

        if (!window)
            window = std::make_unique<sf::RenderWindow>(sf::VideoMode(texture.getSize()), "MJPEG Video Playback");

        sprite.setTexture(texture, true);
        
        window->clear();
        window->draw(sprite);
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

    scheduler.scheduleWorkOnMainLoop(Work([this, 
                                          thisWeakRef = std::move(thisWeakRef)]
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
