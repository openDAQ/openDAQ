#include <testutils/test_helpers.h>
#include <random>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_factory.h>

namespace daq::test_helpers
{

std::string createRandomString(size_t length)
{
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, characters.size() - 1);

    std::string randomString;
    randomString.reserve(length);
    for (size_t i = 0; i < length; ++i)
    {
        randomString += characters[distribution(generator)];
    }
    return randomString;
}

daq::PropertyObjectPtr createRefDeviceConfigWithRandomSerialNumber()
{
    daq::PropertyObjectPtr refDeviceConfig = daq::PropertyObject();
    refDeviceConfig.addProperty(daq::StringProperty("SerialNumber", createRandomString()));
    return refDeviceConfig;
}

} // namespace test_helpers
