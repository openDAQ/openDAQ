/*
 * Copyright 2022-2024 openDAQ d. o. o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <ref_fb_module/common.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/signal_config_ptr.h>
#include <opendaq/block_reader_ptr.h>
#include <opendaq/event_packet_ptr.h>
#include <kiss_fft.h>

BEGIN_NAMESPACE_REF_FB_MODULE
namespace FFT
{

constexpr size_t defaultBlockSize = 1024;
constexpr size_t maxSampleReadCount = 100000;

class FFTFbImpl final : public FunctionBlock
{
public:
    explicit FFTFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~FFTFbImpl() override;

    static FunctionBlockTypePtr CreateType();

private:
    InputPortPtr inputPort;
    bool configValid = false;

    DataDescriptorPtr inputDataDescriptor;
    DataDescriptorPtr inputDomainDataDescriptor;

    DataDescriptorPtr outputDataDescriptor;
    DataDescriptorPtr outputDomainDataDescriptor;

    SampleType inputSampleType;

    SignalConfigPtr outputSignal;
    SignalConfigPtr outputDomainSignal;

    BlockReaderPtr linearReader;

    size_t blockSize;
    size_t maxBlockReadCount;
    std::vector<float> inputData;
    std::vector<uint64_t> inputDomainData;


    kiss_fft_cfg cfg;
    std::vector<kiss_fft_cpx> fftIn;
    std::vector<kiss_fft_cpx> fftOut;

    void createInputPorts();
    void createSignals();

    void calculate();
    void processData(SizeT readAmount);
    void processEventPacket(const EventPacketPtr& packet);

    bool processSignalDescriptorChanged(const DataDescriptorPtr& inputDataDescriptor,
                                        const DataDescriptorPtr& inputDomainDataDescriptor);

    void configure();

    void initProperties();
    void propertyChanged(bool configure);
    void readProperties();
};

}

END_NAMESPACE_REF_FB_MODULE
