#include <opendaq/multi_reader_data_manager.h>

BEGIN_NAMESPACE_OPENDAQ

// Placeholders: every body is to be written against spec 0007

bool MultiReaderDataManager::isSameConfig(const Config& /*config*/)
{
    return false;
}

ErrCode MultiReaderDataManager::reconfigure(Config /*config*/, IMultiReader2Status** /*status*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MultiReaderDataManager::rejudge(IMultiReader2Status** /*status*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

SizeT MultiReaderDataManager::getAvailableCount()
{
    return 0;
}

ErrCode MultiReaderDataManager::read(void** /*data*/, SizeT* /*count*/, SizeT /*timeoutMs*/, SizeT* /*packetOffset*/, IMultiReader2Status** /*status*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MultiReaderDataManager::readWithDomain(void** /*data*/, SizeT* /*count*/, SizeT /*timeoutMs*/, IMultiReader2Status** /*status*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode MultiReaderDataManager::skipSamples(SizeT* /*count*/, SizeT /*timeoutMs*/, SizeT* /*packetOffset*/, IMultiReader2Status** /*status*/)
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

bool MultiReaderDataManager::connected(SizeT /*slot*/, const DataDescriptorPtr& /*value*/, const DataDescriptorPtr& /*domain*/)
{
    return false;
}

bool MultiReaderDataManager::disconnected(SizeT /*slot*/)
{
    return false;
}

bool MultiReaderDataManager::addPacket(SizeT /*slot*/, const PacketPtr& /*packet*/)
{
    return false;
}

bool MultiReaderDataManager::hasTrigger() const
{
    return false;
}

bool MultiReaderDataManager::callFinished(bool /*failed*/)
{
    return false;
}

bool MultiReaderDataManager::inCallback() const
{
    return false;
}

END_NAMESPACE_OPENDAQ
