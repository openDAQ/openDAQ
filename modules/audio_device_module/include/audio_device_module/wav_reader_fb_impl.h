#pragma once
#include <audio_device_module/common.h>
#include <miniaudio/miniaudio.h>
#include <opendaq/data_packet_ptr.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/stream_reader_ptr.h>


BEGIN_NAMESPACE_AUDIO_DEVICE_MODULE

class WAVReaderFbImpl : public FunctionBlock
{
public:
    explicit WAVReaderFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);
    ~WAVReaderFbImpl() override;

    static FunctionBlockTypePtr CreateType();


private:
    bool initializeDecoder();
    bool uninitializeDecoder();
    bool decoderReady();
    bool decoderReading();

    void initProperties();
    bool initializeSignal();

    bool updateFilePath(const std::string& newPath);
    void setRead(bool read);

    void startRead();
    void stopRead();

    DataPacketPtr buildPacket(const void* data, size_t sampleCount);
    void sendPacket(DataPacketPtr packet);

    static void miniaudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

public:
    ma_decoder decoder;

private:
    std::string filePath;

    ma_device device;

    SignalConfigPtr timeSignal;
    SignalConfigPtr outputSignal;

    DataDescriptorPtr domainDescriptor;
    DataDescriptorPtr dataDescriptor;

    Int samplesCaptured;

    bool decoderInitialized;
    bool reading;
    std::atomic<bool> framesAvailable;

    LoggerPtr logger;
    LoggerComponentPtr loggerComponent;
};

END_NAMESPACE_AUDIO_DEVICE_MODULE

