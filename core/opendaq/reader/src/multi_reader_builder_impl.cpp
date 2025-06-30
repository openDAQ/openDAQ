#include <opendaq/multi_reader_builder_impl.h>
#include <opendaq/multi_reader_builder_ptr.h>
#include <opendaq/reader_factory.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

MultiReaderBuilderImpl::MultiReaderBuilderImpl()
    : sources(List<IComponent>())
    , valueReadType(SampleType::Float64)
    , domainReadType(SampleType::Int64)
    , readMode(ReadMode::Scaled)
    , readTimeoutType(ReadTimeoutType::All)
    , requiredCommonSampleRate(-1)
    , startOnFullUnitOfDomain(false)
    , minReadCount(1)
    , offsetTolerance(nullptr)
    , allowDifferentRates(true)
    , notificationMethod(PacketReadyNotification::SameThread)
    , notificationMethodsList(List<PacketReadyNotification>())
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

    sources.pushBack(signal);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::addInputPort(IInputPort* port)
{
    OPENDAQ_PARAM_NOT_NULL(port);

    sources.pushBack(port);
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::addSignals(IList* signals)
{
    OPENDAQ_PARAM_NOT_NULL(signals);

    ListPtr<IBaseObject> list = signals;
    for (const auto& obj : list)
    {
        if (!obj.supportsInterface(ISignal::Id))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Object added to multi reader builder is not a signal.");
        sources.pushBack(obj);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::addInputPorts(IList* inputPorts)
{
    OPENDAQ_PARAM_NOT_NULL(inputPorts);

    ListPtr<IBaseObject> list = inputPorts;
    for (const auto& obj : list)
    {
        if (!obj.supportsInterface(IInputPort::Id))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Object added to multi reader builder is not an input port.");
        sources.pushBack(obj);
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getSourceComponents(IList** ports)
{
    OPENDAQ_PARAM_NOT_NULL(ports);
    *ports = sources.addRefAndReturn();
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

ErrCode MultiReaderBuilderImpl::setMinReadCount(SizeT minReadCount)
{
    this->minReadCount = minReadCount;

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getMinReadCount(SizeT* minReadCount)
{
    OPENDAQ_PARAM_NOT_NULL(minReadCount);

    *minReadCount = this->minReadCount;
    return OPENDAQ_SUCCESS;
}
ErrCode MultiReaderBuilderImpl::setTickOffsetTolerance(IRatio* offsetTolerance)
{
    this->offsetTolerance = offsetTolerance;

    return OPENDAQ_SUCCESS;
}
ErrCode MultiReaderBuilderImpl::getTickOffsetTolerance(IRatio** offsetTolerance)
{
    OPENDAQ_PARAM_NOT_NULL(offsetTolerance);

    *offsetTolerance = this->offsetTolerance.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::setAllowDifferentSamplingRates(Bool allowDifferentRates)
{
    this->allowDifferentRates = allowDifferentRates;

    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getAllowDifferentSamplingRates(Bool* allowDifferentRates)
{
    OPENDAQ_PARAM_NOT_NULL(allowDifferentRates);

    *allowDifferentRates = this->allowDifferentRates;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::setInputPortNotificationMethod(PacketReadyNotification notificationMethod)
{
    this->notificationMethod = notificationMethod;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getInputPortNotificationMethod(PacketReadyNotification* notificationMethod)
{
    OPENDAQ_PARAM_NOT_NULL(notificationMethod);
    *notificationMethod = this->notificationMethod;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::setInputPortNotificationMethods(IList* notificationMethods)
{
    OPENDAQ_PARAM_NOT_NULL(notificationMethods);

    this->notificationMethodsList = notificationMethods;
    return OPENDAQ_SUCCESS;
}

ErrCode MultiReaderBuilderImpl::getInputPortNotificationMethods(IList** notificationMethods)
{
    OPENDAQ_PARAM_NOT_NULL(notificationMethods);

    *notificationMethods = this->notificationMethodsList.addRefAndReturn();
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
