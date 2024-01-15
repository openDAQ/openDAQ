#include "opcuatms_client/objects/tms_client_io_folder_impl.h"
#include "opcuatms_client/objects/tms_client_channel_factory.h"
#include "opcuatms_client/objects/tms_client_io_folder_factory.h"
#include "open62541/daqdevice_nodeids.h"
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
    std::map<uint32_t, ComponentPtr> orderedComponents;
    std::vector<ComponentPtr> unorderedComponents;

    findAndCreateIoFolders(orderedComponents, unorderedComponents);
    findAndCreateChannels(orderedComponents, unorderedComponents);

    auto thisPtr = this->template borrowPtr<FolderConfigPtr>();
    for (const auto& val : orderedComponents)
        try
        {
            thisPtr.addItem(val.second);
        }
        catch (const std::exception& e)
        {
            LOG_W("Failed to add custom component \"{}\" to OpcUA client io folder. Error: {}", val.second.getName(), e.what());
        }
    for (const auto& val : unorderedComponents)
        try
        {
           thisPtr.addItem(val);
        }
        catch (const std::exception& e)
        {
            LOG_W("Failed to add custom component \"{}\" to OpcUA client io folder. Error: {}", val.getName(), e.what());
        }
}

void TmsClientIoFolderImpl::findAndCreateChannels(std::map<uint32_t, ComponentPtr>& orderedComponents, std::vector<ComponentPtr>& unorderedComponents)
{
    const auto& references = getChildReferencesOfType(this->nodeId, OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_CHANNELTYPE));

    for (const auto& [browseName, ref] : references.byBrowseName)
    {
        try
        {
            const auto channelNodeId = OpcUaNodeId(ref->nodeId.nodeId);
            auto thisPtr = this->borrowPtr<FolderConfigPtr>();
            auto tmsClientChannel = TmsClientChannel(this->context, thisPtr, browseName, this->clientContext, channelNodeId);
                
            auto numberInList = this->tryReadChildNumberInList(channelNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedComponents.count(numberInList))
                orderedComponents.insert(std::pair<uint32_t, ComponentPtr>(numberInList, tmsClientChannel));
            else
                unorderedComponents.emplace_back(tmsClientChannel);
        }
        catch(...)
        {
            LOG_W("Failed to find and create channel \"{}\" to OpcUA client", browseName);
        }
    }
}

void TmsClientIoFolderImpl::findAndCreateIoFolders(std::map<uint32_t, ComponentPtr>& orderedComponents, std::vector<ComponentPtr>& unorderedComponents)
{
    const auto& folderReferences = getChildReferencesOfType(this->nodeId, OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_IOCOMPONENTTYPE));

    for (const auto& [browseName, ref] : folderReferences.byBrowseName)
    {
        try
        {
            const auto folderNodeId = OpcUaNodeId(ref->nodeId.nodeId);
            auto thisPtr = this->template borrowPtr<FolderConfigPtr>();
            auto tmsClientFolder = TmsClientIoFolder(this->context, thisPtr, browseName, this->clientContext, folderNodeId);

            auto numberInList = this->tryReadChildNumberInList(folderNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedComponents.count(numberInList))
                orderedComponents.insert(std::pair<uint32_t, ComponentPtr>(numberInList, tmsClientFolder));
            else
                unorderedComponents.emplace_back(tmsClientFolder);
        }
        catch(...)
        {
            LOG_W("Failed to find and create io folder \"{}\" to OpcUA client", browseName);
        }
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
