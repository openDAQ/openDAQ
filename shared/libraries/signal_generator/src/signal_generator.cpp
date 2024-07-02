#include "signal_generator/signal_generator.h"

#include <opendaq/sample_type_traits.h>
#include <opendaq/packet_factory.h>

#include <algorithm>
#include <chrono>

BEGIN_NAMESPACE_OPENDAQ

SignalGenerator::SignalGenerator(const SignalConfigPtr& signal,
                                 std::chrono::time_point<std::chrono::system_clock> absTime)
    : signal(signal)
    , tick(0)
{
    generateFunc = [](uint64_t tick, void* valueOut) {};
    updateFunc = [](SignalGenerator& generator, uint64_t tick) {};
    calculateSampleSize();
    calculateResolutionAndOutputRate();
    calculateAbsStartTick(absTime);
}

void SignalGenerator::setFunction(GenerateSampleFunc function)
{
    this->generateFunc = function;
}

void SignalGenerator::setUpdateFunction(UpdateGeneratorFunc function)
{
    this->updateFunc = function;
}

void SignalGenerator::generateSamplesTo(std::chrono::milliseconds currentTime)
{
    const double msToResolution = (double) resolution.getDenominator() / resolution.getNumerator() / 1000;
    uint64_t currentTick = (double) currentTime.count() * msToResolution / 1000 * outputRate;

    generatePacket(tick, (currentTick - tick) / msToResolution);
    tick = currentTick;
}

void SignalGenerator::generatePacket(uint64_t startTick, size_t sampleCount)
{
    updateFunc(*this, startTick);
    if (sampleCount == 0)
        return;

    Int packetOffset = (Int) getAbsTick(startTick);
    auto dataDescriptor = signal.getDescriptor();
    auto domainDescriptor = signal.getDomainSignal().getDescriptor();
    auto domainPacket = DataPacket(domainDescriptor, sampleCount, (Int) packetOffset);
    auto dataPacket = DataPacketWithDomain(domainPacket, dataDescriptor, sampleCount);

    uint8_t* currentSample = (uint8_t*) dataPacket.getRawData();
    const size_t lastTick = startTick + sampleCount;

    for (uint64_t i = startTick; i < lastTick; i++)
    {
        generateFunc(i, currentSample);
        currentSample += sampleSize;
    }

    signal.sendPacket(dataPacket);
    signal.getDomainSignal().asPtr<ISignalConfig>().sendPacket(domainPacket);
}

SignalConfigPtr SignalGenerator::getSignal()
{
    return signal;
}

void SignalGenerator::calculateSampleSize()
{
    sampleSize = getSampleSize(signal.getDescriptor().getSampleType());
}

void SignalGenerator::calculateResolutionAndOutputRate()
{
    auto domainDescriptor = signal.getDomainSignal().getDescriptor();
    resolution = domainDescriptor.getTickResolution();
    auto rule = domainDescriptor.getRule();

    if (rule.assigned() && rule.getType() == DataRuleType::Linear)
    {
        Int delta = rule.getParameters().get("Delta");
        this->outputRate = (double) resolution.getDenominator() / resolution.getNumerator() / delta;
    }
}

void SignalGenerator::calculateAbsStartTick(std::chrono::time_point<std::chrono::system_clock> absTime)
{
    const uint64_t absTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(absTime.time_since_epoch()).count();
    const double msToResolution = (double) resolution.getDenominator() / resolution.getNumerator() / 1000;
    this->absStartTick = absTimeMs * msToResolution;
}

uint64_t SignalGenerator::getAbsTick(uint64_t currentTick)
{
    return this->absStartTick + currentTick;
}

END_NAMESPACE_OPENDAQ
