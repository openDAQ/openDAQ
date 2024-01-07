#include <ref_fb_module/writer_fb_impl.h>
#include <ref_fb_module/dispatch.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/data_packet.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/packet_factory.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace WriterFb
{

WriterFbImpl::WriterFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    createInputPorts();
    createSignals();
    initProperties();
}

void WriterFbImpl::initProperties()
{
    const auto queueValue = FloatProperty("value", False);
    objPtr.addProperty(queueValue);
    objPtr.getOnPropertyValueWrite("value") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    readProperties();
}

void WriterFbImpl::propertyChanged(bool configure)
{
    std::scoped_lock lock(sync);
    readProperties();
}

void WriterFbImpl::readProperties()
{
    queue.pushBack(objPtr.getPropertyValue("value"));
}

FunctionBlockTypePtr WriterFbImpl::CreateType()
{
    return FunctionBlockType("ref_fb_module_writer", "Writer", "Signal writer");
}

void WriterFbImpl::configure()
{
}

void WriterFbImpl::onPacketReceived(const InputPortPtr& port)
{
    std::scoped_lock lock(sync);
    const auto connection = inputPort.getConnection();
    if (connection.assigned())
        while (connection.dequeue().assigned());

    while (!queue.empty())
    {
        auto domainPacket = DataPacket(outputDomainDataDescriptor, 1);
        auto dataPacket = DataPacketWithDomain(domainPacket, outputDataDescriptor, 1);

        double * val = static_cast<double*>(dataPacket.getData());
        *val = queue.popFront();

        outputSignal.sendPacket(dataPacket);
    }
}

void WriterFbImpl::createInputPorts()
{
    inputPort = createAndAddInputPort("input", PacketReadyNotification::Scheduler);
}

static DataDescriptorPtr CreateDescripton(SampleType type = SampleType::Float64, RangePtr range = Range(-10, 10))
{
    return DataDescriptorBuilder().setName("stub").setSampleType(type).setValueRange(range).build();
}

static DataDescriptorPtr CreateDomainDescripton(std::string epoch = "2023-11-24T00:02:03+00:00")
{
    return DataDescriptorBuilder()
        .setName("domain stub")
        .setSampleType(SampleType::UInt64)
        .setOrigin(epoch)
        .setTickResolution(Ratio(1, 1000000))
        .setRule(LinearDataRule(1000, 0))
        .setUnit(Unit("s", -1, "seconds", "time"))
        .build();
}

void WriterFbImpl::createSignals()
{
    outputSignal = createAndAddSignal(String("output"));
    outputDomainSignal = createAndAddSignal(String("output_domain"));

    outputDataDescriptor = CreateDescripton();
    outputDomainDataDescriptor = CreateDomainDescripton();

    outputSignal.setDescription(outputDataDescriptor);
    outputDomainSignal.setDescription(outputDomainDataDescriptor);

    outputSignal.setDomainSignal(outputDomainSignal);
}

}

END_NAMESPACE_REF_FB_MODULE
