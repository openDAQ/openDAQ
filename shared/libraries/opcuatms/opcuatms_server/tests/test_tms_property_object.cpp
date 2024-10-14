#include "coreobjects/property_object_factory.h"
#include "gtest/gtest.h"
#include "opcuaclient/opcuaclient.h"
#include <opcuaclient/monitored_item_create_request.h>
#include <opcuaclient/subscriptions.h>
#include <future>
#include "coreobjects/property_object_class_ptr.h"
#include "opcuatms_server/objects/tms_server_property_object.h"
#include "tms_server_test.h"
#include <opcuaclient/event_filter.h>
#include <coreobjects/property_factory.h>
#include "opendaq/context_factory.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;
using namespace std::chrono_literals;

class TmsPropertyObjectTest : public TmsServerObjectTest
{
public:
    PropertyObjectPtr createPropertyObject()
    {
        GenericPropertyObjectPtr object = PropertyObject();

        auto intProp = IntProperty("IntProp", 1);
        object.addProperty(intProp);

        return object;
    }
};

TEST_F(TmsPropertyObjectTest, Create)
{
    PropertyObjectPtr propertyObject = createPropertyObject();
    auto tmsPropertyObject = TmsServerPropertyObject(propertyObject, this->getServer(), ctx, tmsCtx);
}

TEST_F(TmsPropertyObjectTest, BaseObjectList)
{
    GenericPropertyObjectPtr object = PropertyObject();
    
    auto list = List<IBaseObject>();
    list.pushBack(1.0);
    list.pushBack(2.0);
    list.pushBack(3.0);
    object.addProperty(ListProperty("ListProp", list));

    auto tmsPropertyObject = TmsServerPropertyObject(object, this->getServer(), ctx, tmsCtx);
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    OpcUaObject<UA_ReadRequest> request;
    request->nodesToReadSize = 1;
    request->nodesToRead = (UA_ReadValueId*) UA_Array_new(1, &UA_TYPES[UA_TYPES_READVALUEID]);
    request->nodesToRead[0].nodeId = nodeId.getDetachedValue();
    request->nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE;

    OpcUaObject<UA_ReadResponse> response = UA_Client_Service_read(client->getLockedUaClient(), *request);
    const auto status = response->responseHeader.serviceResult;
    ASSERT_TRUE(status == UA_STATUSCODE_GOOD);
}

TEST_F(TmsPropertyObjectTest, Register)
{
    PropertyObjectPtr propertyObject = createPropertyObject();

    auto tmsPropertyObject = TmsServerPropertyObject(propertyObject, this->getServer(), ctx, tmsCtx);
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    ASSERT_TRUE(this->getClient()->nodeExists(nodeId));
}

TEST_F(TmsPropertyObjectTest, DISABLED_OnPropertyValueChangeEvent)
{
    PropertyObjectPtr propertyObject = createPropertyObject();

    auto tmsPropertyObject = TmsServerPropertyObject(propertyObject, this->getServer(), ctx, tmsCtx);
    auto nodeId = tmsPropertyObject.registerOpcUaNode();

    OpcUaObject<UA_CreateSubscriptionRequest> request = UA_CreateSubscriptionRequest_default();

    auto subscription = client->createSubscription(request);

    EventMonitoredItemCreateRequest monitoredItem(nodeId);
    EventFilter filter(1);
    filter.setSelectClause(0, SimpleAttributeOperand::CreateMessageValue());

    monitoredItem.setEventFilter(std::move(filter));

    std::promise<void> waitForChangeEvent;
    subscription->monitoredItemsCreateEvent(
        UA_TIMESTAMPSTORETURN_BOTH,
        *monitoredItem,
        [&waitForChangeEvent](
            OpcUaClient* client, Subscription* subContext, MonitoredItem* monContext, size_t nEventFields, UA_Variant* eventFields) {
            waitForChangeEvent.set_value();
        });

    propertyObject.setPropertyValue("IntProp", 2);

    auto future = waitForChangeEvent.get_future();
    ASSERT_NE(future.wait_for(2s), std::future_status::timeout);
}
