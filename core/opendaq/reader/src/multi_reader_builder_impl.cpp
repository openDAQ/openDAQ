#include <opendaq/multi_reader_builder_impl.h>
#include <opendaq/multi_reader_builder_ptr.h>
#include <opendaq/reader_factory.h>
#include <coretypes/validation.h>
#include <opendaq/signal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

MultiReaderBuilderImpl::MultiReaderBuilderImpl()
    : inputPorts(List<IInputPort>())
    , valueReadType(SampleType::Float64)
    , domainReadType(SampleType::Int64)
    , readMode(ReadMode::Scaled)
    , readTimeoutType(ReadTimeoutType::All)
    , requiredCommonSampleRate(-1)
    , startOnFullUnitOfDomain(false)
{
}

ErrCode MultiReaderBuilderImpl::build(IMultiReader** multiReader)
{
    OPENDAQ_PARAM_NOT_NULL(multiReader);

    const auto builderPtr = this->borrowPtr<MultiReaderBuilderPtr>();

    return daqTry([&]()
    {
        *multiReader = MultiReaderFromBuilder(builderPtr).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode MultiReaderBuilderImpl::addSignal(ISignal* signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);

    auto signalPtr = SignalPtr::Borrow(signal);
    auto port = InputPort(signalPtr.getContext(), nullptr, fmt::format("Read signal {}", signalPtr.getLocalId()));
    port.setNotificationMethod(PacketReadyNotification::SameThread);
    port.connect(signalPtr);
    inputPorts.pushBack(port);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::addInputPort(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    auto portPtr = InputPortConfigPtr::Borrow(port);
    portPtr.setNotificationMethod(PacketReadyNotification::Scheduler);
    inputPorts.pushBack(portPtr);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getInputPortList(IList** ports)
{
    OPENDAQ_PARAM_NOT_NULL(ports);
    *ports = inputPorts.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::setValueReadType(SampleType type)
{
    valueReadType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getValueReadType(SampleType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = valueReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::setDomainReadType(SampleType type)
{
    domainReadType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getDomainReadType(SampleType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = domainReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::setReadMode(ReadMode mode)
{
    readMode = mode;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getReadMode(ReadMode* mode)
{
    OPENDAQ_PARAM_NOT_NULL(mode);
    *mode = readMode;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::setReadTimeoutType(ReadTimeoutType type)
{
    readTimeoutType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getReadTimeoutType(ReadTimeoutType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = readTimeoutType;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::setRequiredCommonSampleRate(Int sampleRate)
{
    requiredCommonSampleRate = sampleRate;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getRequiredCommonSampleRate(Int* sampleRate)
{
    OPENDAQ_PARAM_NOT_NULL(sampleRate);
    *sampleRate = requiredCommonSampleRate;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::setStartOnFullUnitOfDomain(Bool enabled)
{
    startOnFullUnitOfDomain = enabled;
    return OPENDAQ_SUCCESS;
}
ErrCode MultiReaderBuilderImpl::getStartOnFullUnitOfDomain(Bool* enabled)
{
    OPENDAQ_PARAM_NOT_NULL(enabled);
    *enabled = startOnFullUnitOfDomain;
    return OPENDAQ_SUCCESS;
}

/////////////////////
////
//// FACTORIES
////
////////////////////

extern "C" ErrCode PUBLIC_EXPORT createMultiReaderBuilder(IMultiReaderBuilder** objTmp)
{
    return daq::createObject<IMultiReaderBuilder, MultiReaderBuilderImpl>(objTmp);
}


END_NAMESPACE_OPENDAQ
