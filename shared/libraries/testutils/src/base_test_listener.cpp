#include <testutils/base_test_listener.h>

void BaseTestListener::OnTestStart(const testing::TestInfo& info)
{
#ifdef OPENDAQ_SKIP_UNSTABLE_TESTS
    const auto testFullName = std::string(info.name());
    const auto unstableTestPrefix = std::string("UNSTABLE_SKIPPED_");
    if (testFullName.find(unstableTestPrefix) == 0)
    {
        ::testing::Test::RecordProperty("Unstable test skipped", true);
        GTEST_SKIP() << "Skipping unstable test: " << info.test_suite_name() << "." << testFullName.substr(unstableTestPrefix.length());
    }
#endif
}

void BaseTestListener::OnTestEnd(const testing::TestInfo& info)
{
}
