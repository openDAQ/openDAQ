#include <copendaq.h>

#include <gtest/gtest.h>

#include "opendaq/context_factory.h"

using COpendaqOpendaqTest = testing::Test;

TEST_F(COpendaqOpendaqTest, ConfigProvider)
{
    ConfigProvider* configProvider = nullptr;
    ConfigProvider_createEnvConfigProvider(&configProvider);
    ASSERT_NE(configProvider, nullptr);
    BaseObject_releaseRef(configProvider);
}

TEST_F(COpendaqOpendaqTest, InstanceAndBuilder)
{
    InstanceBuilder* builder = nullptr;
    InstanceBuilder_createInstanceBuilder(&builder);
    ASSERT_NE(builder, nullptr);

    auto context = daq::NullContext();
    Context* ctx = reinterpret_cast<Context*>(context.getObject());

    Instance* instance = nullptr;
    // Creating an instance with a builder causes memleaks on some compilers
    Instance_createInstance(&instance, ctx, nullptr);
    ASSERT_NE(instance, nullptr);

    BaseObject_releaseRef(builder);
    BaseObject_releaseRef(instance);
}
