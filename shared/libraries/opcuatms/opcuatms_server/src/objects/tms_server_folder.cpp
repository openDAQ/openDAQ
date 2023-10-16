#include "opcuatms_server/objects/tms_server_channel.h"
#include "opcuatms_server/objects/tms_server_folder.h"
#include "opcuatms/converters/variant_converter.h"
#include "open62541/tmsdevice_nodeids.h"
#include "opendaq/io_folder_config.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerFolder::TmsServerFolder(const FolderPtr& object, const OpcUaServerPtr& server, const ContextPtr& context)
    : Super(object, server, context)
{
}

void TmsServerFolder::addChildNodes()
{
    for (auto item : object.getItems())
    {
        auto folder = item.asPtrOrNull<IFolder>();
        auto channel = item.asPtrOrNull<IChannel>();
        auto component = item.asPtrOrNull<IComponent>();

        if (channel.assigned())
        {
            auto tmsChannel = registerTmsObjectOrAddReference<TmsServerChannel>(this->nodeId, channel);
            channels.push_back(std::move(tmsChannel));
        }
        else if (folder.assigned()) // It is important to test for folder last as a channel also is a folder!
        {
            auto tmsFolder = registerTmsObjectOrAddReference<TmsServerFolder>(this->nodeId, folder);
            folders.push_back(std::move(tmsFolder));
        }
        else if (component.assigned())  // It is important to test for component after folder!
        {
            auto tmsComponent = registerTmsObjectOrAddReference<TmsServerComponent<>>(this->nodeId, component);
            components.push_back(std::move(tmsComponent));
        }
        else
        {
            throw daq::NotImplementedException("Unhandled item: " + item.getGlobalId());
        }
    }

    Super::addChildNodes();
}

OpcUaNodeId TmsServerFolder::getTmsTypeId()
{
    if (object.asPtrOrNull<IIoFolderConfig>().assigned())
        return OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_IOCOMPONENTTYPE);
    return OpcUaNodeId(NAMESPACE_TMSDEVICE, UA_TMSDEVICEID_DAQCOMPONENTTYPE);
}

void TmsServerFolder::createNonhierarchicalReferences()
{
    createChildNonhierarchicalReferences(channels);
    createChildNonhierarchicalReferences(folders);
}


END_NAMESPACE_OPENDAQ_OPCUA_TMS
