#include <opendaq/tail_reader_builder_impl.h>
#include <opendaq/tail_reader_builder_ptr.h>
#include <opendaq/reader_factory.h>
#include <opendaq/sample_type_traits.h>
#include <coretypes/validation.h>
#include <opendaq/signal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

TailReaderBuilderImpl::TailReaderBuilderImpl()
    : valueReadType(SampleType::Float64)
    , domainReadType(SampleType::RangeInt64)
    , readMode(ReadMode::Scaled)
    , signal(nullptr)
    , inputPort(nullptr)
    , historySize(1)
    , used(false)
    , skipEvents(false)
{
}

ErrCode TailReaderBuilderImpl::build(ITailReader** tailReader)
{
    OPENDAQ_PARAM_NOT_NULL(tailReader);

    const auto builderPtr = this->borrowPtr<TailReaderBuilderPtr>();

    return daqTry([&]()
    {
        if (used)
            return OPENDAQ_ERR_CREATE_FAILED;

        *tailReader = TailReaderFromBuilder(builderPtr).detach();
        used = true;
        return OPENDAQ_SUCCESS;
    });
}

ErrCode TailReaderBuilderImpl::setSignal(ISignal* signal)
{
    this->signal = signal;
    return OPENDAQ_SUCCESS;
}
ErrCode TailReaderBuilderImpl::getSignal(ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);
    *signal = this->signal.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}
ErrCode TailReaderBuilderImpl::setInputPort(IInputPort* port)
{
    this->inputPort = port;
    return OPENDAQ_SUCCESS;
}
ErrCode TailReaderBuilderImpl::getInputPort(IInputPort** port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    *port = this->inputPort.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode TailReaderBuilderImpl::setValueReadType(SampleType valueReadType)
{
    this->valueReadType = valueReadType;
    return OPENDAQ_SUCCESS;
}
ErrCode TailReaderBuilderImpl::getValueReadType(SampleType* valueReadType)
{
    OPENDAQ_PARAM_NOT_NULL(valueReadType);
    *valueReadType = this->valueReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode TailReaderBuilderImpl::setDomainReadType(SampleType domainReadType)
{
    this->domainReadType = domainReadType;
    return OPENDAQ_SUCCESS;
}
ErrCode TailReaderBuilderImpl::getDomainReadType(SampleType* domainReadType)
{
    OPENDAQ_PARAM_NOT_NULL(domainReadType);
    *domainReadType = this->domainReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode TailReaderBuilderImpl::setHistorySize(SizeT historySize)
{
    this->historySize = historySize;
    return OPENDAQ_SUCCESS;
}
ErrCode TailReaderBuilderImpl::getHistorySize(SizeT* historySize)
{
    OPENDAQ_PARAM_NOT_NULL(historySize);
    *historySize = this->historySize;
    return OPENDAQ_SUCCESS;
}

ErrCode TailReaderBuilderImpl::setReadMode(ReadMode mode)
{
    this->readMode = mode;
    return OPENDAQ_SUCCESS;
}
ErrCode TailReaderBuilderImpl::getReadMode(ReadMode* mode)
{
    OPENDAQ_PARAM_NOT_NULL(mode);
    *mode = this->readMode;
    return OPENDAQ_SUCCESS;
}

ErrCode TailReaderBuilderImpl::setSkipEvents(Bool skipEvents)
{
    this->skipEvents = skipEvents;
    return OPENDAQ_SUCCESS;
}
ErrCode TailReaderBuilderImpl::getSkipEvents(Bool* skipEvents)
{
    OPENDAQ_PARAM_NOT_NULL(skipEvents);
    *skipEvents = this->skipEvents;
    return OPENDAQ_SUCCESS;
}

/////////////////////
////
//// FACTORIES
////
////////////////////

extern "C" ErrCode PUBLIC_EXPORT createTailReaderBuilder(ITailReaderBuilder** objTmp)
{
    return daq::createObject<ITailReaderBuilder, TailReaderBuilderImpl>(objTmp);
}

END_NAMESPACE_OPENDAQ