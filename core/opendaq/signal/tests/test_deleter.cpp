#include <opendaq/deleter_factory.h>
#include <gtest/gtest.h>

using DeleterTest = testing::Test;

TEST_F(DeleterTest, Test)
{
    void* mem = std::malloc(16);

    auto deleter = daq::Deleter([](void* address)
        {
            std::free(address);
        });

    deleter.deleteMemory(mem);
}
