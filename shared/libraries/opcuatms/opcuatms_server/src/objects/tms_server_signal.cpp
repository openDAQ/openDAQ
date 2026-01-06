#include <opcuatms_server/objects/tms_server_signal.h>
#include <opcuatms_server/objects/tms_server_analog_value.h>
#include <opcuatms_server/objects/tms_server_value.h>
#include <opcuatms/converters/variant_converter.h>
#include <open62541/daqbsp_nodeids.h>
#include <open62541/statuscodes.h>
#include <open62541/server.h>
#include <coreobjects/core_event_args_ids.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

using namespace opcua;

TmsServerSignal::TmsServerSignal(const SignalPtr& object,
                                 const OpcUaServerPtr& server,
                                 const ContextPtr& context,
                                 const TmsServerContextPtr& tmsContext)
    : Super(object, server, context, tmsContext)
{
}

OpcUaNodeId TmsServerSignal::getReferenceType()
{
    // TODO UA_DAQBSPID_HASSTATUSSIGNAL
    return OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASVALUESIGNAL);
}

OpcUaNodeId TmsServerSignal::getTmsTypeId()
{
    return OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_SIGNALTYPE);
}

void TmsServerSignal::addChildNodes()
{
    // Create Value and AnalogValue nodes manually with correct data type
    // Store them as members to keep them alive (callbacks depend on them)
    valueServer = std::make_shared<TmsServerValue>(object, server, daqContext, tmsContext);
    valueServer->registerOpcUaNode(nodeId);
    
    analogValueServer = std::make_shared<TmsServerAnalogValue>(object, server, daqContext, tmsContext);
    analogValueServer->registerOpcUaNode(nodeId);
    
    Super::addChildNodes();
}

void TmsServerSignal::onCoreEvent(const CoreEventArgsPtr& args)
{
    Super::onCoreEvent(args);

    if (args.getEventId() == static_cast<int>(CoreEventId::DataDescriptorChanged))
    {
        try
        {
            const auto descriptor = object.getDescriptor();
            if (!descriptor.assigned() || !valueServer)
                return;

            const auto currentDataType = server->readDataType(valueServer->getNodeId());
            const auto expectedDataType = TmsServerValue::SampleTypeToOpcUaDataType(descriptor.getSampleType());

            if (currentDataType == expectedDataType)
                return;

            if (valueServer)
            {
                auto valueNodeId = valueServer->getNodeId();
                if (!valueNodeId.isNull())
                    server->deleteNode(valueNodeId);
            }

            if (analogValueServer)
            {
                auto analogValueNodeId = analogValueServer->getNodeId();
                if (!analogValueNodeId.isNull())
                    server->deleteNode(analogValueNodeId);
            }

            valueServer = std::make_shared<TmsServerValue>(object, server, daqContext, tmsContext);
            valueServer->registerOpcUaNode(nodeId);

            analogValueServer = std::make_shared<TmsServerAnalogValue>(object, server, daqContext, tmsContext);
            analogValueServer->registerOpcUaNode(nodeId);
        }
        catch (...)
        {
        }
    }
}

void TmsServerSignal::createNonhierarchicalReferences()
{
    auto domainSignal = object.getDomainSignal();
    if (domainSignal.assigned())
    {
        auto domainSignalNodeId = findSignalNodeId(domainSignal);
        if (!domainSignalNodeId.isNull())
        {
            try
            {
                addReference(domainSignalNodeId, OpcUaNodeId(NAMESPACE_DAQBSP, UA_DAQBSPID_HASDOMAINSIGNAL));
            }
            catch (const OpcUaException& ex)
            {
                if (ex.getStatusCode() != UA_STATUSCODE_BADDUPLICATEREFERENCENOTALLOWED)
                    throw;
            }
        }
    }
}

END_NAMESPACE_OPENDAQ_OPCUA_TMS
