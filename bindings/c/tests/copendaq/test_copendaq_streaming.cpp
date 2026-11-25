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

TEST_F(COpendaqStreamingTest, TransportOptions)
{
    daqString* plainName = nullptr;
    daqString_createString(&plainName, "plain");
    daqTransportOption* plain = nullptr;
    ASSERT_EQ(daqTransportOption_createTransportOption(&plain, plainName, 7420, daqSecurityModeNone, False), DAQ_SUCCESS);
    ASSERT_NE(plain, nullptr);

    daqString* tlsName = nullptr;
    daqString_createString(&tlsName, "tls");
    daqTransportOption* tls = nullptr;
    ASSERT_EQ(daqTransportOption_createTransportOption(&tls, tlsName, 7422, daqSecurityModeEncrypted, True), DAQ_SUCCESS);

    daqSecurityMode securityMode = daqSecurityModeNone;
    ASSERT_EQ(daqTransportOption_getSecurityMode(tls, &securityMode), DAQ_SUCCESS);
    ASSERT_EQ(securityMode, daqSecurityModeEncrypted);
    daqBool certificateRequired = False;
    ASSERT_EQ(daqTransportOption_getCertificateRequired(tls, &certificateRequired), DAQ_SUCCESS);
    ASSERT_TRUE(certificateRequired);

    // a server capability announces the options it serves, the first one is the default
    daqString* protocolId = nullptr;
    daqString_createString(&protocolId, "protocol_id");
    daqServerCapabilityConfig* capability = nullptr;
    ASSERT_EQ(daqServerCapabilityConfig_createServerCapability(&capability, protocolId, protocolId, daqProtocolTypeStreaming), DAQ_SUCCESS);
    ASSERT_EQ(daqServerCapabilityConfig_addAvailableTransportOption(capability, tls), DAQ_SUCCESS);
    ASSERT_EQ(daqServerCapabilityConfig_addAvailableTransportOption(capability, plain), DAQ_SUCCESS);

    daqServerCapability* serverCapability = nullptr;
    daqBaseObject_borrowInterface(capability, DAQ_SERVER_CAPABILITY_INTF_ID, (void**) &serverCapability);
    // DISCARD PORT: the options are a dict keyed by their ids, in the order added
    daqDict* options = nullptr;
    ASSERT_EQ(daqServerCapability_getAvailableTransportOptions(serverCapability, &options), DAQ_SUCCESS);
    daqSizeT count = 0;
    daqDict_getCount(options, &count);
    ASSERT_EQ(count, 2u);

    daqList* keys = nullptr;
    daqDict_getKeyList(options, &keys);
    daqBaseObject* firstName = nullptr;
    daqList_getItemAt(keys, 0, &firstName);
    daqConstCharPtr firstNameStr = nullptr;
    daqString_getCharPtr((daqString*) firstName, &firstNameStr);
    ASSERT_STREQ(firstNameStr, "tls");
    daqBaseObject* first = nullptr;
    daqDict_get(options, firstName, &first);
    daqTransportOption* firstOption = nullptr;
    daqBaseObject_borrowInterface(first, DAQ_TRANSPORT_OPTION_INTF_ID, (void**) &firstOption);
    daqInt firstPortValue = 0;
    daqTransportOption_getPort(firstOption, &firstPortValue);
    ASSERT_EQ(firstPortValue, 7422);

    // the deprecated port follows the default option
    daqInteger* capabilityPort = nullptr;
    daqServerCapability_getPort(serverCapability, &capabilityPort);
    daqInt capabilityPortValue = 0;
    daqInteger_getValue(capabilityPort, &capabilityPortValue);
    ASSERT_EQ(capabilityPortValue, 7422);

    daqBaseObject_releaseRef(capabilityPort);
    daqBaseObject_releaseRef(keys);
    daqBaseObject_releaseRef(firstName);
    daqBaseObject_releaseRef(first);
    daqBaseObject_releaseRef(options);
    daqBaseObject_releaseRef(capability);
    daqBaseObject_releaseRef(protocolId);
    daqBaseObject_releaseRef(tls);
    daqBaseObject_releaseRef(tlsName);
    daqBaseObject_releaseRef(plain);
    daqBaseObject_releaseRef(plainName);
}
