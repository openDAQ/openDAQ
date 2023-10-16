#include "opcuatms_client/objects/tms_client_io_folder_impl.h"
#include "opcuatms_client/objects/tms_client_channel_factory.h"
#include "opcuatms_client/objects/tms_client_io_folder_factory.h"
#include "open62541/tmsdevice_nodeids.h"
#include "opendaq/folder_config_ptr.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace daq::opcua;

TmsClientIoFolderImpl::TmsClientIoFolderImpl(const ContextPtr& ctx,
                                             const ComponentPtr& parent,
                                             const StringPtr& localId,
                                             const TmsClientContextPtr& clientContext,
                                             const opcua::OpcUaNodeId& nodeId)
    : TmsClientFolderImpl<IoFolderImpl>(ctx, parent, localId, clientContext, nodeId, true)
{
    findAndCreateIoFolders();
    findAndCreateChannels();
}

void TmsClientIoFolderImpl::findAndCreateChannels()
{
    auto channelNodeIds = getChildNodes(this->client, this->nodeId, OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_CHANNELTYPE));
    for (const auto& channelNodeId : channelNodeIds)
    {
        auto browseName = this->client->readBrowseName(channelNodeId);
        auto thisPtr = this->borrowPtr<FolderConfigPtr>();
        auto tmsClientChannel = TmsClientChannel(this->context, thisPtr, browseName, this->clientContext, channelNodeId);
        thisPtr.addItem(tmsClientChannel);
    }
}

void TmsClientIoFolderImpl::findAndCreateIoFolders()
{
    auto folderNodeIds = getChildNodes(this->client, this->nodeId, OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_IOCOMPONENTTYPE));
    for (const auto& folderNodeId : folderNodeIds)
    {
        auto browseName = this->client->readBrowseName(folderNodeId);
        auto thisPtr = this->template borrowPtr<FolderConfigPtr>();
        auto tmsClientFolder = TmsClientIoFolder(this->context, thisPtr, browseName, this->clientContext, folderNodeId);
        thisPtr.addItem(tmsClientFolder);
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
