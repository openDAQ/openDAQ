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
    daqInteger* plainPort = nullptr;
    daqInteger_createInteger(&plainPort, 7420);
    daqTransportOption* plain = nullptr;
    ASSERT_EQ(daqTransportOption_createTransportOption(&plain, plainName, plainPort, daqTransportProtectionNone, False), DAQ_SUCCESS);
    ASSERT_NE(plain, nullptr);

    daqString* tlsName = nullptr;
    daqString_createString(&tlsName, "tls");
    daqInteger* tlsPort = nullptr;
    daqInteger_createInteger(&tlsPort, 7422);
    daqTransportOption* tls = nullptr;
    ASSERT_EQ(daqTransportOption_createTransportOption(&tls, tlsName, tlsPort, daqTransportProtectionEncrypted, True), DAQ_SUCCESS);

    daqTransportProtection protection = daqTransportProtectionNone;
    ASSERT_EQ(daqTransportOption_getProtection(tls, &protection), DAQ_SUCCESS);
    ASSERT_EQ(protection, daqTransportProtectionEncrypted);
    daqBool certificateRequired = False;
    ASSERT_EQ(daqTransportOption_getCertificateRequired(tls, &certificateRequired), DAQ_SUCCESS);
    ASSERT_TRUE(certificateRequired);

    // a server capability announces the options it serves, the first one is the default
    daqString* protocolId = nullptr;
    daqString_createString(&protocolId, "protocol_id");
    daqServerCapabilityConfig* capability = nullptr;
    ASSERT_EQ(daqServerCapabilityConfig_createServerCapability(&capability, protocolId, protocolId, daqProtocolTypeStreaming), DAQ_SUCCESS);
    ASSERT_EQ(daqServerCapabilityConfig_addTransportOption(capability, tls), DAQ_SUCCESS);
    ASSERT_EQ(daqServerCapabilityConfig_addTransportOption(capability, plain), DAQ_SUCCESS);

    daqServerCapability* serverCapability = nullptr;
    daqBaseObject_borrowInterface(capability, DAQ_SERVER_CAPABILITY_INTF_ID, (void**) &serverCapability);
    daqList* options = nullptr;
    ASSERT_EQ(daqServerCapability_getTransportOptions(serverCapability, &options), DAQ_SUCCESS);
    daqSizeT count = 0;
    daqList_getCount(options, &count);
    ASSERT_EQ(count, 2u);

    daqBaseObject* first = nullptr;
    daqList_getItemAt(options, 0, &first);
    daqTransportOption* firstOption = nullptr;
    daqBaseObject_borrowInterface(first, DAQ_TRANSPORT_OPTION_INTF_ID, (void**) &firstOption);
    daqString* firstName = nullptr;
    daqTransportOption_getName(firstOption, &firstName);
    daqConstCharPtr firstNameStr = nullptr;
    daqString_getCharPtr(firstName, &firstNameStr);
    ASSERT_STREQ(firstNameStr, "tls");
    daqInteger* firstPort = nullptr;
    daqTransportOption_getPort(firstOption, &firstPort);
    daqInt firstPortValue = 0;
    daqInteger_getValue(firstPort, &firstPortValue);
    ASSERT_EQ(firstPortValue, 7422);

    // the deprecated port follows the default option
    daqInteger* capabilityPort = nullptr;
    daqServerCapability_getPort(serverCapability, &capabilityPort);
    daqInt capabilityPortValue = 0;
    daqInteger_getValue(capabilityPort, &capabilityPortValue);
    ASSERT_EQ(capabilityPortValue, 7422);

    daqBaseObject_releaseRef(capabilityPort);
    daqBaseObject_releaseRef(firstPort);
    daqBaseObject_releaseRef(firstName);
    daqBaseObject_releaseRef(first);
    daqBaseObject_releaseRef(options);
    daqBaseObject_releaseRef(capability);
    daqBaseObject_releaseRef(protocolId);
    daqBaseObject_releaseRef(tls);
    daqBaseObject_releaseRef(tlsPort);
    daqBaseObject_releaseRef(tlsName);
    daqBaseObject_releaseRef(plain);
    daqBaseObject_releaseRef(plainPort);
    daqBaseObject_releaseRef(plainName);
}
