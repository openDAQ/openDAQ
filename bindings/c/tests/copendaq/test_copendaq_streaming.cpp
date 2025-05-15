#include <copendaq.h>

#include <gtest/gtest.h>

using COpendaqStreamingTest = testing::Test;

TEST_F(COpendaqStreamingTest, StreamingType)
{
    StreamingType* streamingType = nullptr;
    String* id = nullptr;
    String_createString(&id, "streamingType");
    String* name = nullptr;
    String_createString(&name, "streamingTypeName");
    String* description = nullptr;
    String_createString(&description, "streamingTypeDescription");
    String* prefix = nullptr;
    String_createString(&prefix, "streamingTypePrefix");
    StreamingType_createStreamingType(&streamingType, id, name, description, prefix, nullptr);
    ASSERT_NE(streamingType, nullptr);
    BaseObject_releaseRef(streamingType);
    BaseObject_releaseRef(id);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(description);
    BaseObject_releaseRef(prefix);
}

TEST_F(COpendaqStreamingTest, SubscriptionEventArgs)
{
    SubscriptionEventArgs* subscriptionEventArgs = nullptr;
    String* streamingConnectionString = nullptr;
    String_createString(&streamingConnectionString, "streamingConnectionString");
    SubscriptionEventType type = SubscriptionEventType::SubscriptionEventTypeUnsubscribed;

    SubscriptionEventArgs_createSubscriptionEventArgs(&subscriptionEventArgs, streamingConnectionString, type);
    ASSERT_NE(subscriptionEventArgs, nullptr);
    BaseObject_releaseRef(subscriptionEventArgs);
    BaseObject_releaseRef(streamingConnectionString);
}
