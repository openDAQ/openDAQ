/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <opendaq/event_packet_params.h>
#include <coretypes/validation.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/ownable_ptr.h>

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
                        SampleType domainReadType)
        : readMode(mode)
        , timeoutType(ReadTimeoutType::All)
    {
        if (!signal.assigned())
            throw ArgumentNullException("Signal must not be null.");

        this->internalAddRef();

        port = InputPort(signal.getContext(), nullptr, "readsig");
        port.setListener(this->template thisPtr<InputPortNotificationsPtr>());

        port.connect(signal);
        connection = port.getConnection();

        valueReader = createReaderForType(valueReadType, nullptr);
        domainReader = createReaderForType(domainReadType, nullptr);
    }

    explicit ReaderImpl(InputPortConfigPtr port,
                        ReadMode mode,
                        SampleType valueReadType,
                        SampleType domainReadType)
        : readMode(mode)
        , timeoutType(ReadTimeoutType::All)
    {
        if (!port.assigned())
            throw ArgumentNullException("Signal must not be null.");
        
        port.asPtr<IOwnable>().setOwner(portBinder);

        this->internalAddRef();

        this->port = port;
        this->port.setListener(this->template thisPtr<InputPortNotificationsPtr>());

        if (port.getConnection().assigned())
            connection = this->port.getConnection();
        valueReader = createReaderForType(valueReadType, nullptr);
        domainReader = createReaderForType(domainReadType, nullptr);
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
    ErrCode INTERFACE_FUNC connected(IInputPort* inputPort) override
    {
        OPENDAQ_PARAM_NOT_NULL(inputPort);
        
        std::scoped_lock lock(mutex);
        connection = InputPortConfigPtr::Borrow(inputPort).getConnection();
        handleDescriptorChanged(connection.dequeue());
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Called when a signal is disconnected from the input port.
     * @param inputPort The port from which a signal was disconnected.
     */
    ErrCode INTERFACE_FUNC disconnected(IInputPort* inputPort) override
    {
        OPENDAQ_PARAM_NOT_NULL(inputPort);

        std::scoped_lock lock(mutex);
        connection = nullptr;
        return OPENDAQ_SUCCESS;
    }

    /*!
     * @brief Gets the user the option to invalidate the reader when the signal descriptor changes
     * @param callback The callback to call when the descriptor changes. The callback takes a descriptor
     * as a parameter and returns a boolean indicating whether the change is still acceptable.
     */
    ErrCode INTERFACE_FUNC setOnDescriptorChanged(IFunction* callback) override
    {
        std::scoped_lock lock(mutex);

        changeCallback = callback;
        return OPENDAQ_SUCCESS;
    }

    ErrCode INTERFACE_FUNC setOnAvailablePackets(IFunction* callback) override
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
        auto callback = readCallback;
        if (callback.assigned())
            callback();

        return OPENDAQ_SUCCESS;
    }

    // IReaderConfig

    /*!
     * @brief Gets the currently set callback to call when the signal descriptor changes if any.
     * @param[out] callback The callback to call when the descriptor changes or @c nullptr if not set.
     */
    ErrCode INTERFACE_FUNC getOnDescriptorChanged(IFunction** callback) override
    {
        std::scoped_lock lock(mutex);

        *callback = changeCallback.addRefAndReturn();
        return OPENDAQ_SUCCESS;
    }

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
    {
        std::unique_lock lock(old->mutex);
        old->invalid = true;

        timeoutType = old->timeoutType;

        port = old->port;
        connection = old->connection;
        changeCallback = old->changeCallback;
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
        changeCallback = readerConfig.getOnDescriptorChanged();
        connection = port.getConnection();

        valueReader = createReaderForType(valueReadType, readerConfig.getValueTransformFunction());
        domainReader = createReaderForType(domainReadType, readerConfig.getDomainTransformFunction());
    }

    void inferReaderReadType(DataDescriptorPtr newDescriptor, std::unique_ptr<Reader>& reader)
    {
        if (!newDescriptor.assigned())
        {
            invalid = true;
        }
        else
        {
            reader = createReaderForType(newDescriptor.getSampleType(), reader->getTransformFunction());
        }
    }

    virtual void handleDescriptorChanged(const EventPacketPtr& eventPacket)
    {
        if (!eventPacket.assigned())
            return;

        auto params = eventPacket.getParameters();
        DataDescriptorPtr newValueDescriptor = params[event_packet_param::DATA_DESCRIPTOR];
        DataDescriptorPtr newDomainDescriptor = params[event_packet_param::DOMAIN_DATA_DESCRIPTOR];

        // Check if value is stil convertible
        if (newValueDescriptor.assigned())
        {
            if (valueReader->isUndefined())
            {
                inferReaderReadType(newValueDescriptor, valueReader);
            }

            auto valid = valueReader->handleDescriptorChanged(newValueDescriptor);
            if (!invalid)
            {
                invalid = !valid;
            }
        }

        // Check if domain is stil convertible
        if (newDomainDescriptor.assigned())
        {
            if (domainReader->isUndefined())
            {
                inferReaderReadType(newDomainDescriptor, domainReader);
            }

            auto valid = domainReader->handleDescriptorChanged(newDomainDescriptor);
            if (!invalid)
            {
                invalid = !valid;
            }
        }

        // If both value and domain are still convertible
        // check with the user if new state is valid for them
        if (!invalid && changeCallback.assigned())
        {
            bool descriptorOk = false;
            ErrCode errCode = wrapHandlerReturn(changeCallback, descriptorOk, newValueDescriptor, newDomainDescriptor);
            invalid = !descriptorOk || OPENDAQ_FAILED(errCode);

            if (OPENDAQ_FAILED(errCode))
                daqClearErrorInfo();
        }
    }

    void readDescriptorFromPort()
    {
        auto config = port.asPtrOrNull<IInputPortConfig>();
        if (config.assigned())
        {
            config.setListener(this->template thisPtr<InputPortNotificationsPtr>());
        }

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

        const auto signal = port.getSignal();
        if (!signal.assigned())
        {
            throw InvalidStateException("Input port must already have a signal assigned");
        }

        const auto descriptor = signal.getDescriptor();
        if (!descriptor.assigned())
        {
            throw InvalidStateException("Input port connected signal must have a descriptor assigned.");
        }
        handleDescriptorChanged(DataDescriptorChangedEventPacket(descriptor, nullptr));
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

        if (!domainReader->handleDescriptorChanged(dataDescriptor))
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

    bool invalid{};
    std::mutex mutex;
    ReadMode readMode;
    InputPortConfigPtr port;
    PropertyObjectPtr portBinder{PropertyObject()};
    ConnectionPtr connection;
    FunctionPtr changeCallback;
    FunctionPtr readCallback;
    ReadTimeoutType timeoutType;

    std::unique_ptr<Reader> valueReader;
    std::unique_ptr<Reader> domainReader;
};

END_NAMESPACE_OPENDAQ
