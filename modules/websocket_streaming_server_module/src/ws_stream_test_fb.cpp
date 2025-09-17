/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <websocket_streaming_server_module/common.h>
#include <websocket_streaming_server_module/ws_stream_test_fb.h>

using namespace std::chrono;
using namespace std::chrono_literals;

BEGIN_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE

static constexpr unsigned blockRate = 10;

FunctionBlockTypePtr WsStreamTestFb::createType()
{
    return FunctionBlockType(
        ID,
        ID,
        "Exposes signals provided by a WebSocket Streaming client");
}

WsStreamTestFb::WsStreamTestFb(
        const ContextPtr& ctx,
        const ComponentPtr& parent,
        const StringPtr& localId)
    : FunctionBlock(createType(), ctx, parent, localId)
    , _sampleRate(1000)
{
    _domainDescriptor = DataDescriptorBuilder()
            .setName("Time")
            .setSampleType(SampleType::Int64)
            .setUnit(Unit("s", 1, "seconds", "time"))
            .setOrigin("1970-01-01T00:00:00")
            .setRule(LinearDataRule(duration_cast<system_clock::duration>(1s).count() / _sampleRate, 0))
            .setTickResolution(Ratio(system_clock::period::num, system_clock::period::den))
            .build();

    _linearDomainSignal = createAndAddSignal(
        "Time",
        _domainDescriptor);

    _constantValueSignal = createAndAddSignal(
        "Status",
        DataDescriptorBuilder()
            .setName("Status")
            .setSampleType(SampleType::UInt64)
            .setRule(ConstantDataRule())
            .build());
    _constantValueSignal.setDomainSignal(_linearDomainSignal);

    _explicitValueSignal1 = createAndAddSignal(
        "Value1",
        DataDescriptorBuilder()
            .setName("Value1")
            .setSampleType(SampleType::Float64)
            .setUnit(Unit("V", 1, "volts", "voltage"))
            .setValueRange(Range(-1, 1))
            .build());
    _explicitValueSignal1.setDomainSignal(_linearDomainSignal);

    _explicitValueSignal2 = createAndAddSignal(
        "Value2",
        DataDescriptorBuilder()
            .setName("Value2")
            .setSampleType(SampleType::Float64)
            .setUnit(Unit("V", 1, "volts", "voltage"))
            .setValueRange(Range(-1, 1))
            .build());
    _explicitValueSignal2.setDomainSignal(_linearDomainSignal);

    auto do_reconfigure = [this](PropertyObjectPtr&, PropertyValueEventArgsPtr&) { reconfigure(); };

    addProperty(IntProperty("SampleRate", _sampleRate));
    addProperty(BoolProperty("Run", false));
    
    objPtr.getOnPropertyValueWrite("SampleRate") += do_reconfigure;
    objPtr.getOnPropertyValueWrite("Run") += do_reconfigure;
}

void WsStreamTestFb::reconfigure()
{
    bool run = objPtr.getPropertyValue("Run");

    if (run && !_thread.joinable())
    {
        _sampleRate = objPtr.getPropertyValue("SampleRate");

        _domainDescriptor = DataDescriptorBuilder()
            .setName("Time")
            .setSampleType(SampleType::Int64)
            .setUnit(Unit("s", 1, "seconds", "time"))
            .setOrigin("1970-01-01T00:00:00")
            .setRule(LinearDataRule(duration_cast<system_clock::duration>(1s).count() / _sampleRate, 0))
            .setTickResolution(Ratio(system_clock::period::num, system_clock::period::den))
            .build();

        _exit = false;
        _thread = std::thread([this]() { threadFunc(); });
    }

    else if (_thread.joinable())
    {
        _exit = true;
        _thread.join();
    }
}

void WsStreamTestFb::threadFunc()
{
    std::vector<double> samples(_sampleRate / blockRate);
    auto when = system_clock::now();
    std::uint64_t t = 0;

    while (!_exit)
    {
        when += duration_cast<system_clock::duration>(1s) / blockRate;
        std::this_thread::sleep_until(when);

        for (std::size_t i = 0; i < samples.size(); ++i)
            samples[i] = std::sin(++t / static_cast<double>(_sampleRate));

        auto domainPacket = DataPacket(
            _domainDescriptor,
            samples.size(),
            when.time_since_epoch().count());

        auto constantPacket = ConstantDataPacketWithDomain<std::uint64_t>(
            domainPacket,
            _constantValueSignal.getDescriptor(),
            samples.size(),
            42);

        auto explicitPacket1 = DataPacketWithDomain(
            domainPacket,
            _explicitValueSignal1.getDescriptor(),
            samples.size());

        auto explicitPacket2 = DataPacketWithDomain(
            domainPacket,
            _explicitValueSignal2.getDescriptor(),
            samples.size());

        std::memcpy(
            explicitPacket1.getRawData(),
            samples.data(),
            std::min(
                samples.size() * sizeof(decltype(samples)::value_type),
                explicitPacket1.getRawDataSize()));

        std::memcpy(
            explicitPacket2.getRawData(),
            samples.data(),
            std::min(
                samples.size() * sizeof(decltype(samples)::value_type),
                explicitPacket2.getRawDataSize()));

        _linearDomainSignal.sendPacket(domainPacket);
        _constantValueSignal.sendPacket(constantPacket);
        _explicitValueSignal1.sendPacket(explicitPacket1);
        _explicitValueSignal2.sendPacket(explicitPacket2);
    }
}

END_NAMESPACE_OPENDAQ_WEBSOCKET_STREAMING_SERVER_MODULE
