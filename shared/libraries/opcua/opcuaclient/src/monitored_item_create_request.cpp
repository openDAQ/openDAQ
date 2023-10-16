#include <opcuaclient/monitored_item_create_request.h>
#include <open62541/client_subscriptions.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

/*MonitoredItemCreateRequest*/

MonitoredItemCreateRequest::MonitoredItemCreateRequest()
    : OpcUaObject<UA_MonitoredItemCreateRequest>(UA_MonitoredItemCreateRequest_default(UA_NODEID_NULL))
{
}

/*EventMonitoredItemCreateRequest*/

EventMonitoredItemCreateRequest::EventMonitoredItemCreateRequest()
    : MonitoredItemCreateRequest()
{
    value.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    value.monitoringMode = UA_MONITORINGMODE_REPORTING;
}

EventMonitoredItemCreateRequest::EventMonitoredItemCreateRequest(const OpcUaNodeId& nodeId)
    : EventMonitoredItemCreateRequest()
{
    setItemToMonitor(nodeId);
}

void EventMonitoredItemCreateRequest::setItemToMonitor(const OpcUaNodeId& nodeId)
{
    UA_NodeId_clear(&value.itemToMonitor.nodeId);
    value.itemToMonitor.nodeId = nodeId.copyAndGetDetachedValue();
}

void EventMonitoredItemCreateRequest::setEventFilter(const OpcUaObject<UA_EventFilter>& eventFilter)
{
    UA_EventFilter* uaEventFilter = UA_EventFilter_new();
    *uaEventFilter = eventFilter.copyAndGetDetachedValue();

    setEventFilter(uaEventFilter);
}

void EventMonitoredItemCreateRequest::setEventFilter(OpcUaObject<UA_EventFilter>&& eventFilter)
{
    UA_EventFilter* uaEventFilter = UA_EventFilter_new();
    *uaEventFilter = eventFilter.getDetachedValue();

    setEventFilter(uaEventFilter);
}

void EventMonitoredItemCreateRequest::setEventFilter(UA_EventFilter* eventFilter)
{
    UA_ExtensionObject_clear(&value.requestedParameters.filter);

    value.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    value.requestedParameters.filter.content.decoded.data = eventFilter;
    value.requestedParameters.filter.content.decoded.type = &UA_TYPES[UA_TYPES_EVENTFILTER];
}

END_NAMESPACE_OPENDAQ_OPCUA
