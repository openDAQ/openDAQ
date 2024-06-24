#include <ref_fb_module/fft_fb_impl.h>
#include <ref_fb_module/dispatch.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <opendaq/signal_factory.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>
#include <coreobjects/unit_factory.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/event_packet_ids.h>
#include <opendaq/packet_factory.h>
#include <opendaq/range_factory.h>
#include <opendaq/sample_type_traits.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/reader_factory.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace FFT
{

FFTFbImpl::FFTFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
    : FunctionBlock(CreateType(), ctx, parent, localId)
{
    initProperties();
    createSignals();
    createInputPorts();

    cfg = nullptr;
}

FFTFbImpl::~FFTFbImpl()
{
    kiss_fft_free(cfg);
}

void FFTFbImpl::initProperties()
{

    auto blockSizeProp = IntPropertyBuilder("BlockSize", defaultBlockSize).build();
    objPtr.addProperty(blockSizeProp);
    objPtr.getOnPropertyValueWrite("BlockSize") +=
        [this](PropertyObjectPtr& obj, PropertyValueEventArgsPtr& args) { propertyChanged(true); };

    readProperties();
}

void FFTFbImpl::propertyChanged(bool configure)
{
    std::scoped_lock lock(sync);
    readProperties();
    if (configure)
        this->configure();
}

void FFTFbImpl::readProperties()
{
    blockSize = objPtr.getPropertyValue("BlockSize") * 2;
    maxBlockReadCount = maxSampleReadCount / blockSize;

    inputData.resize(maxBlockReadCount * blockSize);
    inputDomainData.resize(maxBlockReadCount * blockSize);
}

FunctionBlockTypePtr FFTFbImpl::CreateType()
{
    return FunctionBlockType("RefFBModuleFFT", "FFT", "Fast Fourier Transform");
}

bool FFTFbImpl::processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor, const DataDescriptorPtr& inputDomainDataDescriptor)
{
    if (inputDataDescriptor.assigned())
        this->inputDataDescriptor = inputDataDescriptor;
    if (inputDomainDataDescriptor.assigned())
        this->inputDomainDataDescriptor = inputDomainDataDescriptor;

    if (!inputDataDescriptor.assigned() && !inputDomainDataDescriptor.assigned())
        return false;

    configure();
    return true;
}

void FFTFbImpl::configure()
{
    configValid = false;
    if (!inputDataDescriptor.assigned())
    {
        LOG_D("FFT: Incomplete input data signal descriptor")
        return;
    }

    if (!inputDomainDataDescriptor.assigned())
    {
        LOG_D("FFT: Incomplete input domain signal descriptor")
        return;
    }

    try
    {
        if (inputDataDescriptor.getSampleType() == SampleType::Struct || inputDataDescriptor.getDimensions().getCount() > 0)
            throw std::runtime_error("Incompatible input value data descriptor");

        inputSampleType = inputDataDescriptor.getSampleType();
        if (inputSampleType != SampleType::Float64 &&
            inputSampleType != SampleType::Float32 &&
            inputSampleType != SampleType::Int8 &&
            inputSampleType != SampleType::Int16 &&
            inputSampleType != SampleType::Int32 &&
            inputSampleType != SampleType::Int64 &&
            inputSampleType != SampleType::UInt8 &&
            inputSampleType != SampleType::UInt16 &&
            inputSampleType != SampleType::UInt32 &&
            inputSampleType != SampleType::UInt64)
            throw std::runtime_error("Invalid sample type");

        if (inputDomainDataDescriptor.getSampleType() != SampleType::Int64 && inputDomainDataDescriptor.getSampleType() != SampleType::UInt64)
        {
            throw std::runtime_error("Incompatible domain data sample type");
        }

        const auto domainUnit = inputDomainDataDescriptor.getUnit();
        if (domainUnit.getSymbol() != "s" && domainUnit.getSymbol() != "seconds")
        {
            throw std::runtime_error("Domain unit expected in seconds");
        }

        const auto domainRule = inputDomainDataDescriptor.getRule();
        if (inputDomainDataDescriptor.getRule().getType() != DataRuleType::Linear)
        {
            throw std::runtime_error("FFT: Domain rule must be linear");
        }

        linearReader = BlockReaderFromExisting(linearReader, blockSize, SampleType::Float32, SampleType::UInt64);

        auto dimensions = List<IDimension>();
        const auto resolution = inputDomainDataDescriptor.getTickResolution();
        if (!resolution.assigned())
        {
            throw std::runtime_error("FFT: Domain signal descriptor has no Tick resolution configured");
        }

        const int delta = domainRule.getParameters().get("Delta");
        const double inverted = static_cast<double>(resolution.getDenominator()) / static_cast<double>(resolution.getNumerator()) / delta;
        const double dimensionDelta = inverted / static_cast<double>(blockSize);
        const auto rule = DimensionRuleBuilder()
                              .setType(DimensionRuleType::Linear)
                              .addParameter("Delta", dimensionDelta)
                              .addParameter("Start", 0)
                              .addParameter("Size", blockSize / 2)
                              .build();
        dimensions.pushBack(Dimension(rule, Unit("Hz", -1, "Hertz"), "Frequency"));
        const auto labels = dimensions[0].getLabels();
        const auto inputRange = inputDataDescriptor.getValueRange();
        outputDataDescriptor = DataDescriptorBuilder()
                                   .setSampleType(SampleType::Float64)
                                   .setDimensions(dimensions)
                                   .setUnit(inputDataDescriptor.getUnit())
                                   .setValueRange(inputRange.assigned() ? Range(0, inputRange.getHighValue()) : Range(0, 10))
                                   .build();

        outputSignal.setDescriptor(outputDataDescriptor);

        outputDomainDataDescriptor =
            DataDescriptorBuilderCopy(inputDomainDataDescriptor).setRule(ExplicitDataRule()).setSampleType(SampleType::UInt64).build();
        outputDomainSignal.setDescriptor(outputDomainDataDescriptor);

        kiss_fft_free(cfg);
        cfg = kiss_fft_alloc(static_cast<int>(blockSize), 0, nullptr, nullptr);
        fftIn.resize(blockSize);
        fftOut.resize(blockSize);

        configValid = true;
    }
    catch (const std::exception& e)
    {
        LOG_W("FFT: Failed to set descriptor for signal: {}", e.what())
        outputSignal.setDescriptor(nullptr);
    }
}

void FFTFbImpl::processEventPacket(const EventPacketPtr& packet)
{
    if (packet.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
    {
        const DataDescriptorPtr dataDesc = packet.getParameters().get(event_packet_param::DATA_DESCRIPTOR);
        const DataDescriptorPtr domainDesc = packet.getParameters().get(event_packet_param::DOMAIN_DATA_DESCRIPTOR);
        processSignalDescriptorChanged(dataDesc, domainDesc);
    }
}

void FFTFbImpl::calculate()
{
    std::scoped_lock lock(sync);
    SizeT availableBlocks = linearReader.getAvailableCount();
    bool continueReading = availableBlocks > 0;

    while (continueReading)
    {
        SizeT readAmount = std::min(availableBlocks, maxBlockReadCount);
        const auto status = linearReader.readWithDomain(inputData.data(), inputDomainData.data(), &readAmount);

        if (configValid)
        {
            processData(readAmount);
        }

        if (status.getReadStatus() == ReadStatus::Event)
        {
            const auto eventPacket = status.getEventPacket();
            if (eventPacket.assigned())
                processEventPacket(eventPacket);
            return;
        }

        continueReading = readAmount < availableBlocks;
        availableBlocks -= readAmount;
    }

}

void FFTFbImpl::processData(SizeT readAmount)
{
    if (readAmount == 0)
        return;

    const auto outputDomainPacket = DataPacket(outputDomainDataDescriptor, readAmount);
    const auto outputDomainData = static_cast<uint64_t*>(outputDomainPacket.getData());

    const auto outputPacket = DataPacketWithDomain(outputDomainPacket, outputDataDescriptor, readAmount);
    const auto outputData = static_cast<double*>(outputPacket.getData());

    for (size_t blockIdx = 0; blockIdx < readAmount; blockIdx++)
    {
        for (size_t i = 0; i < blockSize; i++)
        {
            fftIn[i].r = inputData[blockIdx * blockSize + i];
            fftIn[i].i = 0;
        }

        kiss_fft(cfg, fftIn.data(), fftOut.data());

        for (size_t idx = 0; idx < blockSize / 2; idx++)
            outputData[blockIdx * (blockSize / 2) + idx] = 2 * std::sqrt(std::pow(fftOut[idx + 1].r, 2) + std::pow(fftOut[idx + 1].i, 2)) / blockSize;
        outputDomainData[blockIdx] = inputDomainData[blockIdx * blockSize];
    }

    outputSignal.sendPacket(outputPacket);
    outputDomainSignal.sendPacket(outputDomainPacket);
}

void FFTFbImpl::createInputPorts()
{
    inputPort = createAndAddInputPort("Input", PacketReadyNotification::Scheduler);

    linearReader = BlockReaderFromPort(inputPort, blockSize, SampleType::Float32, SampleType::UInt64);
    linearReader.setOnDataAvailable([this] { calculate();});
}

void FFTFbImpl::createSignals()
{
    outputSignal = createAndAddSignal("FFT_Ampl");
    outputDomainSignal = createAndAddSignal("FFT_Domain", nullptr, false);
    outputSignal.setDomainSignal(outputDomainSignal);
}

}

END_NAMESPACE_REF_FB_MODULE
