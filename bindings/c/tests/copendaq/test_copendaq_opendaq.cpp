#include <copendaq.h>

#include <gtest/gtest.h>

#include "opendaq/context_factory.h"

using COpendaqOpendaqTest = testing::Test;

TEST_F(COpendaqOpendaqTest, ConfigProvider)
{
    daqConfigProvider* configProvider = nullptr;
    daqConfigProvider_createEnvConfigProvider(&configProvider);
    ASSERT_NE(configProvider, nullptr);
    daqBaseObject_releaseRef(configProvider);
}

TEST_F(COpendaqOpendaqTest, InstanceAndBuilder)
{
    daqInstanceBuilder* builder = nullptr;
    daqInstanceBuilder_createInstanceBuilder(&builder);
    ASSERT_NE(builder, nullptr);

    auto context = daq::NullContext();
    daqContext* ctx = (daqContext*) context.getObject();

    daqInstance* instance = nullptr;
    // Creating an instance with a builder causes memleaks on some compilers
    daqInstance_createInstance(&instance, ctx, nullptr);
    ASSERT_NE(instance, nullptr);

    daqBaseObject_releaseRef(builder);
    daqBaseObject_releaseRef(instance);
}
