#include <opendaq/block_reader_builder_impl.h>
#include <opendaq/block_reader_builder_ptr.h>
#include <opendaq/reader_factory.h>
#include <opendaq/sample_type_traits.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

BlockReaderBuilderImpl::BlockReaderBuilderImpl()
    : valueReadType(SampleType::Float64)
    , domainReadType(SampleTypeFromType<ClockTick>::SampleType)
    , readMode(ReadMode::Scaled)
    , blockSize(0)
    , overlap(0)
    , signal(nullptr)
    , inputPort(nullptr)
    , used(false)
    , skipEvents(false)
{
}

ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::build(IBlockReader** blockReader)
{
    OPENDAQ_PARAM_NOT_NULL(blockReader);

    const auto builderPtr = this->borrowPtr<BlockReaderBuilderPtr>();

    return daqTry([&]()
    {
        if (used)
            return OPENDAQ_ERR_CREATE_FAILED;

        *blockReader = BlockReaderFromBuilder(builderPtr).detach();
        used = true;
        return OPENDAQ_SUCCESS;
    });
}

ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::setOldBlockReader(IBlockReader* blockReader)
{
    this->oldBlockReader = blockReader;
    return OPENDAQ_SUCCESS;
}
ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::getOldBlockReader(IBlockReader** blockReader)
{
    OPENDAQ_PARAM_NOT_NULL(blockReader);
    *blockReader = this->oldBlockReader.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::setSignal(ISignal* signal)
{
    this->signal = signal;
    return OPENDAQ_SUCCESS;
}
ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::getSignal(ISignal** signal)
{
    OPENDAQ_PARAM_NOT_NULL(signal);
    *signal = this->signal.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}
ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::setInputPort(IInputPort* port)
{
    this->inputPort = port;
    return OPENDAQ_SUCCESS;
}
ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::getInputPort(IInputPort** port)
{
    OPENDAQ_PARAM_NOT_NULL(port);
    *port = this->inputPort.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::setValueReadType(SampleType valueReadType)
{
    this->valueReadType = valueReadType;
    return OPENDAQ_SUCCESS;
}
ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::getValueReadType(SampleType* valueReadType)
{
    OPENDAQ_PARAM_NOT_NULL(valueReadType);
    *valueReadType = this->valueReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::setDomainReadType(SampleType domainReadType)
{
    this->domainReadType = domainReadType;
    return OPENDAQ_SUCCESS;
}
ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::getDomainReadType(SampleType* domainReadType)
{
    OPENDAQ_PARAM_NOT_NULL(domainReadType);
    *domainReadType = this->domainReadType;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::setReadMode(ReadMode mode)
{
    this->readMode = mode;
    return OPENDAQ_SUCCESS;
}
ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::getReadMode(ReadMode* mode)
{
    OPENDAQ_PARAM_NOT_NULL(mode);
    *mode = this->readMode;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::setBlockSize(SizeT size)
{
    this->blockSize = size;
    return OPENDAQ_SUCCESS;
}
ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::getBlockSize(SizeT* size)
{
    OPENDAQ_PARAM_NOT_NULL(size);
    *size = this->blockSize;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::setOverlap(SizeT overlap)
{
    this->overlap = overlap;
    return OPENDAQ_SUCCESS;
}
ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::getOverlap(SizeT* overlap)
{
    OPENDAQ_PARAM_NOT_NULL(overlap);
    *overlap = this->overlap;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::setSkipEvents(Bool skipEvents)
{
    this->skipEvents = skipEvents;
    return OPENDAQ_SUCCESS;
}
ErrCode INTERFACE_FUNC BlockReaderBuilderImpl::getSkipEvents(Bool* skipEvents)
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

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, BlockReaderBuilder, IBlockReaderBuilder, createBlockReaderBuilder,
)

END_NAMESPACE_OPENDAQ