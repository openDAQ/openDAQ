#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqStreamingTest = testing::Test;

TEST_F(COpendaqStreamingTest, StreamingType)
{
    daqStreamingType* streamingType = nullptr;
    daqString* id = nullptr;
    daqString_createString(&id, "streamingType");
    daqString* name = nullptr;
    daqString_createString(&name, "streamingTypeName");
    daqString* description = nullptr;
    daqString_createString(&description, "streamingTypeDescription");
    daqString* prefix = nullptr;
    daqString_createString(&prefix, "streamingTypePrefix");
    daqStreamingType_createStreamingType(&streamingType, id, name, description, prefix, nullptr, nullptr, nullptr);
    ASSERT_NE(streamingType, nullptr);
    daqBaseObject_releaseRef(streamingType);
    daqBaseObject_releaseRef(id);
    daqBaseObject_releaseRef(name);
    daqBaseObject_releaseRef(description);
    daqBaseObject_releaseRef(prefix);
}

TEST_F(COpendaqStreamingTest, SubscriptionEventArgs)
{
    daqSubscriptionEventArgs* subscriptionEventArgs = nullptr;
    daqString* streamingConnectionString = nullptr;
    daqString_createString(&streamingConnectionString, "streamingConnectionString");
    daqSubscriptionEventType type = daqSubscriptionEventType::daqSubscriptionEventTypeUnsubscribed;

    daqSubscriptionEventArgs_createSubscriptionEventArgs(&subscriptionEventArgs, streamingConnectionString, type);
    ASSERT_NE(subscriptionEventArgs, nullptr);
    daqBaseObject_releaseRef(subscriptionEventArgs);
    daqBaseObject_releaseRef(streamingConnectionString);
}

TEST_F(COpendaqStreamingTest, AddStreamingConfig)
{
    daqList* transports = nullptr;
    daqList_createList(&transports);
    daqString* plain = nullptr;
    daqString_createString(&plain, "plain");
    daqList_pushBack(transports, plain);
    daqString* tls = nullptr;
    daqString_createString(&tls, "tls");
    daqList_pushBack(transports, tls);

    daqAddStreamingConfig* config = nullptr;
    ASSERT_EQ(daqAddStreamingConfig_createAddStreamingConfig(&config, transports), DAQ_SUCCESS);
    ASSERT_NE(config, nullptr);

    daqAddComponentConfig* componentConfig = nullptr;
    daqBaseObject_borrowInterface(config, DAQ_ADD_COMPONENT_CONFIG_INTF_ID, (daqBaseObject**) &componentConfig);
    ASSERT_NE(componentConfig, nullptr);

    ASSERT_EQ(daqAddComponentConfig_setTransport(componentConfig, tls), DAQ_SUCCESS);

    daqString* transport = nullptr;
    daqAddComponentConfig_getTransport(componentConfig, &transport);
    daqConstCharPtr transportStr = nullptr;
    daqString_getCharPtr(transport, &transportStr);
    ASSERT_STREQ(transportStr, "tls");

    daqBaseObject_releaseRef(transport);
    daqBaseObject_releaseRef(config);
    daqBaseObject_releaseRef(tls);
    daqBaseObject_releaseRef(plain);
    daqBaseObject_releaseRef(transports);
}
