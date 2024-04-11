#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using HowToGetLastValue = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/howto_guides/pages/howto_measure_single_value.adoc
TEST_F(HowToGetLastValue, GetLastValueSignalFloat64)
{
    const auto signal = Signal(NullContext(), nullptr, "sig");

    // The Data Descriptor for Float64
    auto descriptor = DataDescriptorBuilder().setSampleType(SampleType::Float64).build();

    auto dataPacket = DataPacket(descriptor, 5);
    auto data = static_cast<double*>(dataPacket.getData());
    data[4] = 6.66;
    signal.sendPacket(dataPacket);

    // START DOCS CODE

    // Get last value of a Signal
    auto lastValue = signal.getLastValue();

    // END DOCS CODE

    // Cast to IFloat
    auto number = lastValue.asPtr<IFloat>();
    ASSERT_EQ(number, 6.66);
}

END_NAMESPACE_OPENDAQ
