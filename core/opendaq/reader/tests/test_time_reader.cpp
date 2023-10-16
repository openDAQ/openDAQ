#include <opendaq/reader_factory.h>
#include <testutils/testutils.h>
#include "reader_common.h"
#include <opendaq/time_reader.h>
#include <coreobjects/unit_factory.h>

using namespace daq;

class TimeReaderTest : public ReaderTest<>
{
public:
    using ValueType = int64_t;

    auto createPacketWithDomain(SizeT numSamples, SizeT offset, const DataDescriptorPtr& descriptorDomain) const
    {
        auto domainPacket = daq::DataPacket(descriptorDomain, numSamples, offset);

        return daq::DataPacketWithDomain(domainPacket, this->signal.getDescriptor(), numSamples);
    }

    template <typename Reader>
    void readData(Reader& reader, SizeT blockSize = 1)
    {
        constexpr const SizeT NUM_SAMPLES = 5;
        auto packet = createPacketWithDomain(NUM_SAMPLES * blockSize, 0, createDomainDescriptor());

        auto* ptr = static_cast<ValueType*>(packet.getData());
        for (SizeT i = 0; i < NUM_SAMPLES * blockSize; ++i)
        {
            ptr[i] = static_cast<ValueType>(0);
        }

        this->sendPacket(packet);

        SizeT count{3};
        auto values = std::make_unique<double[]>(count * blockSize);
        auto domain = std::make_unique<std::chrono::system_clock::time_point[]>(count * blockSize);
        reader.readWithDomain(values.get(), domain.get(), &count);

        using namespace date;

        for (SizeT i = 0; i < count * blockSize; ++i)
        {
            std::cout << domain[i] << std::endl;
        }

        std::cout << "--------" << std::endl;

        SizeT count2{2};
        auto values2 = std::make_unique<double[]>(count2 * blockSize);
        auto domain2 = std::make_unique<std::chrono::system_clock::time_point[]>(count2 * blockSize);
        reader.readWithDomain(values2.get(), domain2.get(), &count2);

        for (SizeT i = 0; i < count2 * blockSize; ++i)
        {
            std::cout << domain2[i] << std::endl;
        }
    }
};

TEST_F(TimeReaderTest, StreamReader)
{
    this->signal.setDescriptor(setupDescriptor(SampleTypeFromType<ValueType>::SampleType));
    TimeReader reader(StreamReader(this->signal));

    readData(reader);
}

TEST_F(TimeReaderTest, ReadWithWrappedReader)
{
    this->signal.setDescriptor(setupDescriptor(SampleTypeFromType<ValueType>::SampleType));
    auto streamReader = StreamReader(this->signal);
    TimeReader reader(streamReader);

    readData(streamReader);
}

TEST_F(TimeReaderTest, TailReader)
{
    this->signal.setDescriptor(setupDescriptor(SampleTypeFromType<ValueType>::SampleType));
    TimeReader reader(TailReader(this->signal, 5));

    readData(reader);
}

TEST_F(TimeReaderTest, BlockReader)
{
    this->signal.setDescriptor(setupDescriptor(SampleTypeFromType<ValueType>::SampleType));

    constexpr SizeT BLOCK_SIZE = 2;
    TimeReader reader(BlockReader(this->signal, 2));

    readData(reader, BLOCK_SIZE);
}
