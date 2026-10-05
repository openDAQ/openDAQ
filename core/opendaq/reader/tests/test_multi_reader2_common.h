/*
 * Shared fixture of the MultiReader2 suites. The implementation is compiled into the test binary so the tests can
 * reach the data manager (sync limits) through MultiReader2Impl::getDataManager.
 */
#pragma once
#include <coretypes/event_wrapper.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/multi_reader2_impl.h>
#include <opendaq/multi_reader2_params_impl.h>
#include <opendaq/multi_reader2_params_ptr.h>
#include <opendaq/multi_reader2_ptr.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/multi_reader2_input_status_ptr.h>
#include <opendaq/multi_reader_data_manager.h>
#include "reader_common.h"

#include <gmock/gmock-matchers.h>

#include <array>
#include <atomic>
#include <chrono>
#include <thread>

namespace multi_reader2_test
{
    // A value signal with a packet cadence; the value at tick t is t, so alignment is visible in the values
    struct ReadSignal2
    {
        ReadSignal2(const SignalConfigPtr& signal, Int packetOffset, Int packetSize)
            : packetSize(packetSize)
            , packetOffset(packetOffset)
            , signal(signal)
        {
        }

        DataDescriptorPtr domainDescriptor() const
        {
            return signal.getDomainSignal().getDescriptor();
        }

        template <typename T = double>
        DataPacketPtr createAndSendPacket(Int packetIndex) const
        {
            const Int delta = domainDescriptor().getRule().getParameters()["delta"];
            const Int offset = packetOffset + packetSize * delta * packetIndex;
            auto domainPacket = DataPacket(domainDescriptor(), static_cast<SizeT>(packetSize), offset);
            auto packet = DataPacketWithDomain(domainPacket, signal.getDescriptor(), static_cast<SizeT>(packetSize));
            const auto data = static_cast<T*>(packet.getRawData());
            for (Int i = 0; i < packetSize; i++)
                data[i] = static_cast<T>(static_cast<double>(offset + i * delta));
            signal.sendPacket(packet);
            return packet;
        }

        // A packet at an explicit tick, for gaps and skips
        template <typename T = double>
        void sendAt(Int offset, Int size) const
        {
            const Int delta = domainDescriptor().getRule().getParameters()["delta"];
            auto domainPacket = DataPacket(domainDescriptor(), static_cast<SizeT>(size), offset);
            auto packet = DataPacketWithDomain(domainPacket, signal.getDescriptor(), static_cast<SizeT>(size));
            const auto data = static_cast<T*>(packet.getRawData());
            for (Int i = 0; i < size; i++)
                data[i] = static_cast<T>(static_cast<double>(offset + i * delta));
            signal.sendPacket(packet);
        }

        Int packetSize;
        Int packetOffset;
        SignalConfigPtr signal;
    };
}

class MultiReader2Test : public ReaderTest<>
{
public:
    using ReadSignal2 = multi_reader2_test::ReadSignal2;

    ReadSignal2& addSignal(Int packetOffset, Int packetSize, const SignalPtr& domain, SampleType valueType = SampleType::Float64)
    {
        auto newSignal = Signal(context, nullptr, fmt::format("sig{}", counter++));
        newSignal.setDescriptor(setupDescriptor(valueType));
        newSignal.setDomainSignal(domain);
        return readSignals.emplace_back(newSignal, packetOffset, packetSize);
    }

    SignalConfigPtr createDomainSignal(std::string epoch = "", const RatioPtr& resolution = nullptr, const DataRulePtr& rule = nullptr)
    {
        auto domain = Signal(context, nullptr, fmt::format("time{}", domainCounter++));
        domain.setDescriptor(createDomainDescriptor(std::move(epoch), resolution, rule, nullptr));
        return domain;
    }

    ListPtr<IComponent> signalsToList() const
    {
        auto list = List<IComponent>();
        for (const auto& read : readSignals)
            list.pushBack(read.signal);
        return list;
    }

    ListPtr<IComponent> portsList()
    {
        ports.clear();
        auto list = List<IComponent>();
        for (SizeT i = 0; i < readSignals.size(); i++)
        {
            auto port = InputPort(context, nullptr, fmt::format("port{}", i));
            ports.push_back(port);
            list.pushBack(port);
        }
        return list;
    }

    // Connection edges are boundaries: they evaluate in a read, so the probe makes them take effect
    void connectAll(const MultiReader2Ptr& reader)
    {
        for (SizeT i = 0; i < ports.size(); i++)
            ports[i].connect(readSignals[i].signal);
        scheduler.waitAll();
        probe(reader);
    }

    void sendPackets(Int index)
    {
        for (const auto& read : readSignals)
            read.createAndSendPacket(index);
        scheduler.waitAll();
    }

    static MultiReader2ParamsPtr params(const ListPtr<IComponent>& inputs, SampleType readType = SampleType::Float64)
    {
        MultiReader2ParamsPtr p = createWithImplementation<IMultiReader2Params, MultiReader2ParamsImpl>();
        p.setInputs(inputs);
        p.setValueReadType(readType);
        return p;
    }

    static MultiReader2Ptr createReader(const MultiReader2ParamsPtr& p)
    {
        return MultiReader2Ptr(createWithImplementation<IMultiReader2, MultiReader2Impl>(static_cast<IMultiReader2Params*>(p)));
    }

    // The first status reports everything as new and delivers no data; tests that start with data take it here
    MultiReader2Ptr createReaderProbed(const MultiReader2ParamsPtr& p)
    {
        auto reader = createReader(p);
        scheduler.waitAll();
        probe(reader);
        return reader;
    }

    static MultiReaderDataManager& manager(const MultiReader2Ptr& reader)
    {
        return dynamic_cast<MultiReader2Impl*>(reader.getObject())->getDataManager();
    }

    // The status probe
    static MultiReader2StatusPtr probe(const MultiReader2Ptr& reader)
    {
        SizeT count = 0;
        SizeT offset = 0;
        return reader.read(nullptr, &count, offset);
    }

    // Reads up to `count` doubles per input into `values`, returns the status
    template <SizeT Inputs, SizeT Samples>
    static MultiReader2StatusPtr read(const MultiReader2Ptr& reader, std::array<std::array<double, Samples>, Inputs>& values, SizeT& count, SizeT* offset = nullptr)
    {
        void* buffers[Inputs];
        for (SizeT i = 0; i < Inputs; i++)
            buffers[i] = values[i].data();
        SizeT localOffset = 0;
        return reader.read(buffers, &count, offset ? *offset : localOffset);
    }

    static MultiReader2InputStatusPtr input(const MultiReader2StatusPtr& status, SizeT index)
    {
        return status.getInputs()[index];
    }

protected:
    int counter{};
    int domainCounter{};
    std::vector<ReadSignal2> readSignals;
    std::vector<InputPortConfigPtr> ports;
};

