#include <testutils/base_test_listener.h>

void BaseTestListener::OnTestStart(const testing::TestInfo& info)
{
#ifdef OPENDAQ_SKIP_FLAKY_TESTS
    const auto testFullName = std::string(info.name());
    const auto flakyTestPrefix = std::string("FLAKY_SKIPPED_");
    if (testFullName.find(flakyTestPrefix) == 0)
    {
        ::testing::Test::RecordProperty("Flaky test skipped", true);
        GTEST_SKIP() << "Skipping flaky test: " << info.test_suite_name() << "." << testFullName.substr(flakyTestPrefix.length());
    }
#endif
}

void BaseTestListener::OnTestEnd(const testing::TestInfo& info)
{
}
