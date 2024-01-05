#include <ref_fb_module/writer_fb_impl.h>
#include <ref_fb_module/dispatch.h>
#include <opendaq/input_port_factory.h>
// #include <opendaq/data_descriptor_ptr.h>
// #include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>
#include <opendaq/custom_log.h>
// #include <opendaq/event_packet_params.h>
// #include <coreobjects/unit_factory.h>
#include <opendaq/data_packet.h>
#include <opendaq/data_packet_ptr.h>
// #include <opendaq/event_packet_ids.h>
#include <opendaq/packet_factory.h>
// #include <opendaq/range_factory.h>
// #include <opendaq/sample_type_traits.h>
// #include <coreobjects/eval_value_factory.h>
// #include <opendaq/dimension_factory.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace WritterFb
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
    return FunctionBlockType("ref_fb_module_writter", "Wriiter", "Signal writter");
}

void WriterFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                                   const DataDescriptorPtr& inputDomainDataDescriptor)
{
}

void WriterFbImpl::configure()
{
}

void WriterFbImpl::onPacketReceived(const InputPortPtr& port)
{
    std::scoped_lock lock(sync);
    while (connection.dequeue().assigned());

    while (!queue.empty())
    {
        auto domainPacket = DataPacket(getInputDomainSignal().getDescriptor(), 1);
        auto dataPacket = DataPacketWithDomain(domainPacket, getInputSignal().getDescriptor(), 1);

        double * val = static_cast<double*>(dataPacket.getData());
        *val = queue.popFront();

        outputSignal.sendPacket(dataPacket);
    }
}

void WriterFbImpl::processEventPacket(const EventPacketPtr& packet)
{
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
    outputSignal.setDescription(CreateDescripton());
    outputDomainSignal = createAndAddSignal(String("output_domain"));
    outputDomainSignal.setDescription(CreateDomainDescripton());
    outputSignal.setDomainSignal(outputDomainSignal);
}

}

END_NAMESPACE_REF_FB_MODULE
