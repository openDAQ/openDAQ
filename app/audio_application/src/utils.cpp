#include <audio_application/utils.h>

DeviceInfoList filterDevicesInfos(const DeviceInfoList& deviceInfoList, const std::string& prefix)
{
    auto filteredDeviceInfoList = daq::List<daq::IDeviceInfo>();
    for (const auto& deviceInfoItem : deviceInfoList)
    {
        std::string connectionString = deviceInfoItem.getConnectionString();
        if (connectionString.rfind(prefix, 0) == 0)
            filteredDeviceInfoList.pushBack(deviceInfoItem);
    }

    return filteredDeviceInfoList;
}

void printDevices(std::ostream& stream, const DeviceInfoList& deviceInfoList)
{
    size_t i = 0;
    for (const auto& devInfo: deviceInfoList)
    {
        stream << i++ << ": " << devInfo.getName() << std::endl;
    }
}

bool getLastValue(const daq::PacketReaderPtr& packetReader, float& lastValue)
{
    auto packets = packetReader.readAll();
    size_t j = packets.getCount();
    while (j > 0)
    {
        auto curPacket = packets[--j].asPtrOrNull<daq::IDataPacket>();
        if (curPacket.assigned() && curPacket.getDomainPacket().assigned())
        {
            float* samples = static_cast<float*>(curPacket.getData()) + curPacket.getSampleCount();
            lastValue = *(--samples);
            return true;
        }
    }
    return false;
}

float getNormalizedLogValue(float value)
{
    constexpr float maxValue = 1.0f;
    constexpr float minValue = 0.01f;

    const float minScaleValue = std::log10(minValue);
    const float maxScaleValue = std::log10(maxValue);
    value = std::min(maxValue, std::max(minValue, value));
    const float scaleValue = std::log10(value);

    const float normalizedValue = (scaleValue - minScaleValue) / (maxScaleValue - minScaleValue);
    return normalizedValue;
}

void printValueBar(std::ostream& stream, float normalizedValue, size_t maxVal)
{
    const size_t relVal = static_cast<size_t>(std::round(normalizedValue * maxVal));
    std::string dots(relVal, 'o');
    std::string empty(maxVal - relVal, '.');
    stream << " [" << dots << empty << "]" << '\r' << std::flush;
}

void hideCursor(std::ostream& stream)
{
    stream << "\33[?25l" << std::endl;
}

void printLastValueBar(std::ostream& stream, const daq::PacketReaderPtr& packetReader)
{
    float lastValue;
    if (getLastValue(packetReader, lastValue))
    {
        const float normalizedValue = getNormalizedLogValue(lastValue);
        printValueBar(stream, normalizedValue, 72);
    }
}


