#include <opendaq/stream_reader_builder_impl.h>
#include <opendaq/stream_reader_builder_ptr.h>
#include <opendaq/reader_factory.h>
#include <opendaq/sample_type_traits.h>
#include <coretypes/validation.h>
#include <opendaq/signal_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

StreamReaderBuilderImpl::StreamReaderBuilderImpl()
    : valueReadType(SampleType::Float64)
    , domainReadType(SampleType::RangeInt64)
    , readMode(ReadMode::Scaled)
    , signal(nullptr)
    , inputPort(nullptr)
    , readTimeoutType(ReadTimeoutType::All)
    , used(false)
    , skipEvents(false)
{
}

ErrCode StreamReaderBuilderImpl::build(IStreamReader** streamReader)
{
    OPENDAQ_PARAM_NOT_NULL(streamReader);

    const auto builderPtr = this->borrowPtr<StreamReaderBuilderPtr>();

    return daqTry([&]()
    {
        if (used)
            return OPENDAQ_ERR_CREATE_FAILED;

        *streamReader = StreamReaderFromBuilder(builderPtr).detach();
        used = true;
        return OPENDAQ_SUCCESS;
    });
}

ErrCode StreamReaderBuilderImpl::setSignal(ISignal* signal)
{
    this->signal = signal;
    return OPENDAQ_SUCCESS;
}
ErrCode StreamReaderBuilderImpl::getSignal(ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);
    *signal = this->signal.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}
ErrCode StreamReaderBuilderImpl::setInputPort(IInputPort* port)
{
    this->inputPort = port;
    return OPENDAQ_SUCCESS;
}
ErrCode StreamReaderBuilderImpl::getInputPort(IInputPort** port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    *port = this->inputPort.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderBuilderImpl::setValueReadType(SampleType valueReadType)
{
    this->valueReadType = valueReadType;
    return OPENDAQ_SUCCESS;
}
ErrCode StreamReaderBuilderImpl::getValueReadType(SampleType* valueReadType)
{
    OPENDAQ_PARAM_NOT_NULL(valueReadType);
    *valueReadType = this->valueReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderBuilderImpl::setDomainReadType(SampleType domainReadType)
{
    this->domainReadType = domainReadType;
    return OPENDAQ_SUCCESS;
}
ErrCode StreamReaderBuilderImpl::getDomainReadType(SampleType* domainReadType)
{
    OPENDAQ_PARAM_NOT_NULL(domainReadType);
    *domainReadType = this->domainReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderBuilderImpl::setReadTimeoutType(ReadTimeoutType type)
{
    readTimeoutType = type;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderBuilderImpl::getReadTimeoutType(ReadTimeoutType* type)
{
    OPENDAQ_PARAM_NOT_NULL(type);
    *type = readTimeoutType;
    return OPENDAQ_SUCCESS;
}


ErrCode StreamReaderBuilderImpl::setReadMode(ReadMode mode)
{
    this->readMode = mode;
    return OPENDAQ_SUCCESS;
}
ErrCode StreamReaderBuilderImpl::getReadMode(ReadMode* mode)
{
    OPENDAQ_PARAM_NOT_NULL(mode);
    *mode = this->readMode;
    return OPENDAQ_SUCCESS;
}

ErrCode StreamReaderBuilderImpl::setSkipEvents(Bool skipEvents)
{
    this->skipEvents = skipEvents;
    return OPENDAQ_SUCCESS;
}
ErrCode StreamReaderBuilderImpl::getSkipEvents(Bool* skipEvents)
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

extern "C" ErrCode PUBLIC_EXPORT createStreamReaderBuilder(IStreamReaderBuilder** objTmp)
{
    return daq::createObject<IStreamReaderBuilder, StreamReaderBuilderImpl>(objTmp);
}


END_NAMESPACE_OPENDAQ