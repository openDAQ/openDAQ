/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/sample_reader.h>
#include <opendaq/reader_config_ptr.h>
#include <opendaq/input_port_factory.h>
#include <opendaq/packet_factory.h>
#include <opendaq/typed_reader.h>
#include <opendaq/event_packet_utils.h>
#include <coretypes/validation.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/ownable_ptr.h>
#include <coretypes/number_ptr.h>
#include <opendaq/data_descriptor_factory.h>

#include <mutex>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

template <typename Interface = IReader>
class ReaderImpl;

template <typename Interface>
class ReaderImpl : public ImplementationOfWeak<Interface, IReaderConfig, IInputPortNotifications>
{
public:
    explicit ReaderImpl(SignalPtr signal,
                        ReadMode mode,
                        SampleType valueReadType,
                        SampleType domainReadType,
                        Bool skipEvents)
        : readMode(mode)
        , timeoutType(ReadTimeoutType::All)
        , skipEvents(skipEvents)
    {
        if (!signal.assigned())
            throw ArgumentNullException("Signal must not be null.");

        this->internalAddRef();

        port = InputPort(signal.getContext(), nullptr, "readsig", true);
        port.setListener(this->template thisPtr<InputPortNotificationsPtr>());

        port.connect(signal);
        connection = port.getConnection();

        valueReader = createReaderForType(valueReadType, nullptr);
        domainReader = createReaderForType(domainReadType, nullptr);
    }

    explicit ReaderImpl(InputPortConfigPtr port,
                        ReadMode mode,
                        SampleType valueReadType,
                        SampleType domainReadType,
                        Bool skipEvents)
        : readMode(mode)
        , portBinder(PropertyObject())
        , timeoutType(ReadTimeoutType::All)
        , skipEvents(skipEvents)
    {
        if (!port.assigned())
            throw ArgumentNullException("Port must not be null.");
        
        port.asPtr<IOwnable>().setOwner(portBinder);

        this->internalAddRef();

        this->port = port;
        this->port.setListener(this->template thisPtr<InputPortNotificationsPtr>());
        connection = port.getConnection();

        valueReader = createReaderForType(valueReadType, nullptr);
        domainReader = createReaderForType(domainReadType, nullptr);
    }

    ~ReaderImpl() override
    {
        if (port.assigned() && !portBinder.assigned())
            port.remove();
    }

    /*!
     * @brief Called when the Input port method `acceptsSignal` is called. Should return true if the signal is
     * accepted; false otherwise.
     * @param inputPort The input port on which the method was called.
     * @param signal The signal which is being checked for acceptance.
     * @param[out] accept True if the signal is accepted; false otherwise.
     */
    ErrCode INTERFACE_FUNC acceptsSignal(IInputPort* inputPort, ISignal* signal, Bool* accept) override
    {
        OPENDAQ_PARAM_NOT_NULL(inputPort);
        OPENDAQ_PARAM_NOT_NULL(signal);
        OPENDAQ_PARAM_NOT_NULL(accept);

        *accept = true;
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Called when a signal is connected to the input port.
     * @param inputPort The port to which the signal was connected.
     */
    virtual ErrCode INTERFACE_FUNC connected(IInputPort* inputPort) override
    {
        OPENDAQ_PARAM_NOT_NULL(inputPort);
        inputPort->getConnection(&connection);
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Called when a signal is disconnected from the input port.
     * @param inputPort The port from which a signal was disconnected.
     */
    virtual ErrCode INTERFACE_FUNC disconnected(IInputPort* inputPort) override
    {
        OPENDAQ_PARAM_NOT_NULL(inputPort);

        std::scoped_lock lock(mutex);
        connection = nullptr;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setOnDataAvailable(IProcedure* callback) override
    {
        std::scoped_lock lock(mutex);

        readCallback = callback;
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Notifies the listener of the newly received packet on the specified input-port.
     * @param port The port on which the new packet was received.
     */
    virtual ErrCode INTERFACE_FUNC packetReceived(IInputPort* port) override
    {
        OPENDAQ_PARAM_NOT_NULL(port);
        ProcedurePtr callback;

        {
            std::scoped_lock lock(mutex);
            callback = readCallback;
        }

        if (callback.assigned())
            return wrapHandler(callback);
        return OPENDAQ_SUCCESS;
    }

    // IReaderConfig
    /*!
     * @brief Gets the internally created input-ports if used.
     * @param[out] ports The internal Input-Ports if used by the reader.
     */
    ErrCode INTERFACE_FUNC getInputPorts(IList** ports) override
    {
        OPENDAQ_PARAM_NOT_NULL(ports);

        *ports = nullptr;
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Gets the type of time-out handling used by the reader.
     * @param[out] timeout How the reader handles time-outs.
     */
    ErrCode INTERFACE_FUNC getReadTimeoutType(ReadTimeoutType* timeout) override
    {
        OPENDAQ_PARAM_NOT_NULL(timeout);

        *timeout = timeoutType;
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Marks the current reader as invalid preventing any additional operations to be performed on the reader
     * except reusing its info and configuration in a new reader.
     */
    ErrCode INTERFACE_FUNC markAsInvalid() override
    {
        std::scoped_lock lock(mutex);
        invalid = true;

        return OPENDAQ_SUCCESS;
    }

    // ISampleReader

    /*!
     * @brief Gets the sample-type the signal value samples will be converted to when read
     * or @c SampleType::Invalid if read-type has not been determined yet.
     * @param[out] sampleType The sample-type type of the read samples otherwise @c SampleType::Invalid.
     */
    ErrCode INTERFACE_FUNC getValueReadType(SampleType* sampleType) override
    {
        OPENDAQ_PARAM_NOT_NULL(sampleType);

        std::unique_lock lock(mutex);

        *sampleType = valueReader->getReadType();
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Gets the sample-type the signal domain samples will be converted to when read
     * or @c SampleType::Invalid if read-type has not been determined yet.
     * @param[out] sampleType The sample-type type of the read samples otherwise @c SampleType::Invalid.
     */
    ErrCode INTERFACE_FUNC getDomainReadType(SampleType* sampleType) override
    {
        OPENDAQ_PARAM_NOT_NULL(sampleType);

        std::unique_lock lock(mutex);

        *sampleType = domainReader->getReadType();
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Gets the transform function that will be called with the read domain-data and currently valid Signal-Descriptor
     * giving the user the chance add a custom post-processing step.
     * @param transform The function performing the post-processing or @c nullptr if not assigned.
     */
    ErrCode INTERFACE_FUNC getDomainTransformFunction(IFunction** transform) override
    {
        std::scoped_lock lock(mutex);

        *transform = domainReader->getTransformFunction().addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Sets the transform function that will be called with the read domain-data and currently valid Signal-Descriptor
     * giving the user the chance add a custom post-processing step. The function should have a signature compatible with:
     * @code
     * transform(TransformInfo* info, DataDescriptor descriptor)
     * @endcode
     * @param transform The function performing the post-processing.
     */
    ErrCode INTERFACE_FUNC setDomainTransformFunction(IFunction* transform) override
    {
        std::scoped_lock lock(mutex);

        domainReader->setTransformFunction(transform);
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Gets the transform function that will be called with the read value-data and currently valid Signal-Descriptor
     * giving the user the chance add a custom post-processing step.
     * @param transform The function performing the post-processing or @c nullptr if not assigned.
     */
    ErrCode INTERFACE_FUNC getValueTransformFunction(IFunction** transform) override
    {
        std::scoped_lock lock(mutex);

        *transform = valueReader->getTransformFunction().addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Sets the transform function that will be called with the read value-data and currently valid Signal-Descriptor
     * giving the user the chance add a custom post-processing step. The function should have a signature compatible with:
     * @code
     * transform(TransformInfo* info, DataDescriptor descriptor)
     * @endcode
     * @param transform The function performing the post-processing.
     */
    ErrCode INTERFACE_FUNC setValueTransformFunction(IFunction* transform) override
    {
        std::scoped_lock lock(mutex);

        valueReader->setTransformFunction(transform);
        return OPENDAQ_SUCCESS;
    }

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif

    /*!
     * @brief Gets the reader's read mode which determines if the reader will also scale the read data or not.
     * @param mode The mode the reader is in (either Raw or Scaled)
     */
    ErrCode INTERFACE_FUNC getReadMode(ReadMode* mode)
    {
        OPENDAQ_PARAM_NOT_NULL(mode);

        *mode = readMode;
        return OPENDAQ_SUCCESS;
    }

#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

protected:
    explicit ReaderImpl(ReaderImpl* old,
                        SampleType valueReadType,
                        SampleType domainReadType)
        : readMode(old->readMode)
        , valueReader(daq::createReaderForType(valueReadType, old->valueReader->getTransformFunction()))
        , domainReader(daq::createReaderForType(domainReadType, old->domainReader->getTransformFunction()))
        , skipEvents(old->skipEvents)
    {
        old->invalid = true;
        timeoutType = old->timeoutType;
        portBinder = old->portBinder;
        port = old->port;
        connection = old->connection;
        readCallback = old->readCallback;

        port.asPtr<IOwnable>().setOwner(portBinder);

        this->internalAddRef();
        
        port.setListener(this->template thisPtr<InputPortNotificationsPtr>());
        if (connection.assigned())
            readDescriptorFromPort();
    }

    explicit ReaderImpl(const ReaderConfigPtr& readerConfig,
                        ReadMode mode,
                        SampleType valueReadType,
                        SampleType domainReadType)

        : readMode(mode)
        , timeoutType(ReadTimeoutType::All)
    {
        if (!readerConfig.assigned())
            throw ArgumentNullException("Existing reader must not be null");

        if (!port.assigned())
            throw ArgumentNullException("Input port must not be null");

        readerConfig.markAsInvalid();

        port = readerConfig.getInputPorts()[0];
        connection = port.getConnection();

        valueReader = createReaderForType(valueReadType, readerConfig.getValueTransformFunction());
        domainReader = createReaderForType(domainReadType, readerConfig.getDomainTransformFunction());

        this->internalAddRef();
        port.setListener(this->template thisPtr<InputPortNotificationsPtr>());
        if (connection.assigned())
            readDescriptorFromPort();
    }

    void inferReaderReadType(DataDescriptorPtr newDescriptor, std::unique_ptr<Reader>& reader)
    {
        if (!newDescriptor.assigned())
        {
            invalid = true;
        }
        else
        {
            SampleType readType;
            auto postScaling = newDescriptor.getPostScaling();
            if (!postScaling.assigned() || readMode == ReadMode::Scaled)
            {
                readType = newDescriptor.getSampleType();
            }
            else
            {
                readType = postScaling.getInputSampleType();
            }

            reader = createReaderForType(readType, reader->getTransformFunction());
        }
    }

    virtual void handleDescriptorChanged(const EventPacketPtr& eventPacket)
    {
        if (!eventPacket.assigned())
            return;

        auto [valueDescriptorChanged, domainDescriptorChanged, newValueDescriptor, newDomainDescriptor] =
            parseDataDescriptorEventPacket(eventPacket);

        // Check if value is still convertible
        if (valueDescriptorChanged && newValueDescriptor.assigned())
        {
            if (valueReader->isUndefined())
            {
                inferReaderReadType(newValueDescriptor, valueReader);
            }

            auto valid = valueReader->handleDescriptorChanged(newValueDescriptor, readMode);
            if (!invalid)
            {
                invalid = !valid;
            }
        }

        // Check if domain is still convertible
        if (domainDescriptorChanged && newDomainDescriptor.assigned())
        {
            if (domainReader->isUndefined())
            {
                inferReaderReadType(newDomainDescriptor, domainReader);
            }

            auto valid = domainReader->handleDescriptorChanged(newDomainDescriptor, readMode);
            if (!invalid)
            {
                invalid = !valid;
            }
        }
    }

    void readDescriptorFromPort()
    {
        PacketPtr packet = connection.peek();
        if (packet.assigned() && packet.getType() == PacketType::Event)
        {
            auto eventPacket = packet.asPtr<IEventPacket>(true);
            if (eventPacket.getEventId() == event_packet_id::DATA_DESCRIPTOR_CHANGED)
            {
                handleDescriptorChanged(connection.dequeue());
                return;
            }
        }
    }

    bool trySetDomainSampleType(const daq::DataPacketPtr& domainPacket)
    {
        ObjectPtr<IErrorInfo> errInfo;
        daqGetErrorInfo(&errInfo);
        daqClearErrorInfo();

        auto dataDescriptor = domainPacket.getDataDescriptor();
        if (domainReader->isUndefined())
        {
            inferReaderReadType(dataDescriptor, domainReader);
        }

        if (!domainReader->handleDescriptorChanged(dataDescriptor, readMode))
        {
            daqSetErrorInfo(errInfo);
            return false;
        }
        return true;
    }

    void* getValuePacketData(const DataPacketPtr& packet) const
    {
        switch (readMode)
        {
            case ReadMode::Unscaled:
            case ReadMode::RawValue:
                return packet.getRawData();
            case ReadMode::Scaled:
                return packet.getData();
        }

        throw InvalidOperationException("Unknown Reader read-mode of {}", static_cast<std::underlying_type_t<ReadMode>>(readMode));
    }

    NumberPtr calculateOffset(const DataPacketPtr& packet, SizeT offset) const 
    {
        const auto domainPacket = packet.getDomainPacket();
        if (domainPacket.assigned() && domainPacket.getOffset().assigned())
        {
            Int delta = 0;
            const auto domainRule = domainPacket.getDataDescriptor().getRule();
            if (domainRule.assigned() && domainRule.getType() == DataRuleType::Linear)
            {
                const auto domainRuleParams = domainRule.getParameters();
                delta = domainRuleParams.get("delta");
            }
            return NumberPtr(domainPacket.getOffset().getIntValue() + (offset * delta));
        }
        return NumberPtr(0);
    }

    bool invalid{};
    std::mutex mutex;
    ReadMode readMode;
    InputPortConfigPtr port;
    PropertyObjectPtr portBinder;
    ConnectionPtr connection;
    ProcedurePtr readCallback;
    ReadTimeoutType timeoutType;

    std::unique_ptr<Reader> valueReader;
    std::unique_ptr<Reader> domainReader;
    Bool skipEvents = false;
};

END_NAMESPACE_OPENDAQ
