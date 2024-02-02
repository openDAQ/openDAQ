#include <opcuatms_client/objects/tms_client_component_factory.h>
#include <opcuatms_client/objects/tms_client_folder_factory.h>
#include "opcuatms_client/objects/tms_client_folder_impl.h"
#include "opendaq/io_folder_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace daq::opcua;

template <class Impl>
TmsClientFolderImpl<Impl>::TmsClientFolderImpl(const ContextPtr& ctx,
                                               const ComponentPtr& parent,
                                               const StringPtr& localId,
                                               const TmsClientContextPtr& clientContext,
                                               const opcua::OpcUaNodeId& nodeId,
                                               bool customFolderType)
    : TmsClientComponentBaseImpl<Impl>(ctx, parent, localId, clientContext, nodeId)
    , loggerComponent(this->daqContext.getLogger().assigned() ? this->daqContext.getLogger().getOrAddComponent("OpcUaClientFolder")
                                                              : throw ArgumentNullException("Logger must not be null"))
{
    if (!customFolderType)
    {
        std::map<uint32_t, ComponentPtr> orderedComponents;
        std::vector<ComponentPtr> unorderedComponents;
        findAndCreateFolders(orderedComponents, unorderedComponents);
        auto thisPtr = this->template borrowPtr<FolderConfigPtr>();
        for (const auto& val : orderedComponents)
            thisPtr.addItem(val.second);
        for (const auto& val : unorderedComponents)
            thisPtr.addItem(val);
    }
}

template <class Impl>
void TmsClientFolderImpl<Impl>::findAndCreateFolders(std::map<uint32_t, ComponentPtr>& orderedComponents, std::vector<ComponentPtr>& unorderedComponents)
{
    auto componentId = OpcUaNodeId(NAMESPACE_DAQDEVICE, UA_DAQDEVICEID_DAQCOMPONENTTYPE);
    const auto& folderReferences = this->getChildReferencesOfType(this->nodeId, componentId);

    for (const auto& [browseName, ref] : folderReferences.byBrowseName)
    {
        try
        {
            const auto folderNodeId = OpcUaNodeId(ref->nodeId.nodeId);
            auto thisPtr = this->template borrowPtr<FolderConfigPtr>();

            const auto& childComponentsReferences = this->getChildReferencesOfType(folderNodeId, componentId);

            ComponentPtr child;
            if (!childComponentsReferences.byNodeId.empty())
                child = TmsClientFolder(this->context, thisPtr, browseName, this->clientContext, folderNodeId);
            else
                child = TmsClientComponent(this->context, thisPtr, browseName, this->clientContext, folderNodeId);
        
            auto numberInList = this->tryReadChildNumberInList(folderNodeId);
            if (numberInList != std::numeric_limits<uint32_t>::max() && !orderedComponents.count(numberInList))
                orderedComponents.insert(std::pair<uint32_t, ComponentPtr>(numberInList, child));
            else
                unorderedComponents.push_back(child);
        }
        catch (...)
        {
            LOG_W("Failed to find and create folder \"{}\" to OpcUA client folder \"{}\"", browseName, this->globalId);
            throw;
        }
    }
}

template class TmsClientFolderImpl<FolderImpl<IFolderConfig>>;
template class TmsClientFolderImpl<IoFolderImpl<>>;

END_NAMESPACE_OPENDAQ_OPCUA_TMS
