/*!
 * Connects to available devices, writing their signal data to a .csv file, alongside their timestamps
 * that are obtained from the signal's domain signal. The timestamps are relative to the start of the measurement,
 * starting at 0ms. This example assumes all domain signals are in the time domain, and their data is in seconds.
 */

#include <opendaq/opendaq.h>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <thread>

using namespace std::chrono_literals;

class Recorder
{
public:
    using TimeReader = std::unique_ptr<daq::TimeReader<daq::StreamReaderPtr>>;

    daq::StreamReaderPtr reader;
    TimeReader timeReader;

    daq::DataDescriptorPtr dataDescriptor;
    daq::DataDescriptorPtr domainDataDescriptor;

    std::vector<daq::Float> buffer;
    std::vector<uint64_t> ticksBuffer;
    std::vector<std::chrono::system_clock::time_point> timeBuffer;
    daq::RatioPtr resolutionForMs;
    size_t read;
    uint64_t startTick = std::numeric_limits<uint64_t>::max();

    Recorder(const daq::SignalPtr& signal, size_t bufferSize, bool readTime)
        : dataDescriptor(signal.getDescriptor())
        , domainDataDescriptor(signal.getDomainSignal().getDescriptor())
        , read(0)
    {
        reader = daq::StreamReader<daq::Float, uint64_t>(signal);
        buffer.resize(bufferSize);

        if (readTime)
        {
            timeReader = std::make_unique<daq::TimeReader<daq::StreamReaderPtr>>(reader);
            timeBuffer.resize(bufferSize);
        }
        else
        {
            ticksBuffer.resize(bufferSize);
        }

        // We're assuming the domain signal is in seconds here.
        assert(domainDataDescriptor.getUnit().getSymbol() == "s");
        auto resolution = domainDataDescriptor.getTickResolution();
        resolutionForMs = resolution.assigned() ? resolution * static_cast<daq::Int>(1000) : daq::Ratio(1000, 1);
    }

    daq::Int toUnixTimestampInMs(uint64_t tick) const
    {
        // Ticks are scaled with resolution to a floating point value (in seconds). Here we round to an integer.
        // The ticks are relative to the signal's origin (`domainDataDescriptor.getOrigin()`).
        return std::lround<daq::Int>(static_cast<daq::Int>(tick) * resolutionForMs);
    }

    daq::Int deltaUnixTimestampInMs(uint64_t tick) const
    {
        return std::lround<daq::Int>(static_cast<daq::Int>(tick - startTick) * resolutionForMs);
    }

    size_t readSamples()
    {
        read = buffer.size();

        if (timeReader)
        {
            timeReader->readWithDomain(buffer.data(), timeBuffer.data(), &read);
        }
        else
        {
            reader.readWithDomain(buffer.data(), ticksBuffer.data(), &read);
        }
        return read;
    }

    void clearBuffer()
    {
        this->read = 0;
    }

    void printSamples(size_t maxToPrint) const
    {
        std::cout << dataDescriptor.getName() << ": ";
        maxToPrint = std::min(read, maxToPrint);
        for (size_t i = 0; i < maxToPrint; i++)
            std::cout << std::fixed << std::setprecision(2) << buffer[i] << " ";
        std::cout << std::endl;
    }
};

class CsvWriter;
using CsvWriterPtr = std::shared_ptr<CsvWriter>;

class CsvWriter
{
public:
    std::ofstream file;
    std::vector<Recorder>* recorders;

    CsvWriter(const std::string& filename, std::vector<Recorder>& recorders)
        : file(filename)
        , recorders(&recorders)
    {
    }

    ~CsvWriter()
    {
        close();
    }

    void writeBuffers()
    {
        size_t maxRead = 0;
        for (auto& recorder : *recorders)
            maxRead = std::max<size_t>(maxRead, recorder.read);

        for (size_t i = 0; i < maxRead; i++)
        {
            for (auto& recorder : *recorders)
                writeSample(recorder, i);
            file << "\n";
        }

        for (auto& recorder : *recorders)
            recorder.clearBuffer();
    }

    void writeSample(Recorder& recorder, size_t index)
    {
        if (index < recorder.read)
        {
            using namespace date;

            static bool firstWrite = true;
            if (firstWrite)
            {
                if (recorder.timeReader)
                {
                    file << date::format("%F %T (%Z)", recorder.timeBuffer[index]) << ";\n";
                }
                else
                {
                    daq::Int unixTimestamp = recorder.toUnixTimestampInMs(recorder.ticksBuffer[index]);

                    int ms = unixTimestamp % 1000;
                    int s = (int) (unixTimestamp / 1000) % 60;
                    int m = (int) (unixTimestamp / (1000 * 60)) % 60;
                    int h = (int) (unixTimestamp / (1000 * 60 * 60)) % 24;
                    file << h << "h:" << m << "m:" << s << "s:" << ms << "ms;"
                         << "\n";
                }
            }

            if (recorder.timeReader)
            {
                file << date::format("%F %T (%Z)", recorder.timeBuffer[index]) << ";"
                     << std::fixed << std::setprecision(2) << recorder.buffer[index] << ";";
            }
            else
            {
                if (recorder.startTick == std::numeric_limits<uint64_t>::max())
                {
                    recorder.startTick = recorder.ticksBuffer[index];
                }

                daq::Int unixTimestamp = recorder.deltaUnixTimestampInMs(recorder.ticksBuffer[index]);
                file << unixTimestamp << "ms;" << std::fixed << std::setprecision(2) << recorder.buffer[index] << ";";
            }

            firstWrite = false;
        }
        else
        {
            file << ";";
        }
    }

    void writeHeader()
    {
        // Add signal name
        for (auto& recorder : *recorders)
            file << recorder.dataDescriptor.getName() << ";;";

        file << "\n";
    }

    void close()
    {
        if (file.is_open())
            file.close();
    }
};

class RecordedDevice;
using RecordedDevicePtr = std::shared_ptr<RecordedDevice>;

class RecordedDevice
{
public:
    std::string name;
    std::vector<Recorder> recorders;
    CsvWriterPtr writer;

    RecordedDevice(const std::string& name, std::vector<Recorder> recorders)
        : name(name)
        , recorders(std::move(recorders))
    {
        std::string filename = name + ".csv";
        std::replace(filename.begin(), filename.end(), ':', '_');
        this->writer = std::make_shared<CsvWriter>(filename, this->recorders);
    }
};

void printSamples(const std::string& channelName, const std::vector<daq::Float>& buffer, size_t maxToPrint)
{
    std::cout << channelName << ": ";
    maxToPrint = std::min(buffer.size(), maxToPrint);
    for (size_t i = 0; i < maxToPrint; i++)
        std::cout << buffer[i] << " ";
    std::cout << std::endl;
}

int parseArgs(int argc, const char* argv[], daq::Int& websocketPort, bool& useRenderer, std::string& connectionString)
{
    std::string arg1 = (argc >= 2) ? argv[1] : "";
    if (arg1 == "-h")
    {
        std::cout << "HELP: tms_test_app.exe [websocketPort=4714] [renderSignals=false] [connectionString=any]" << std::endl;
        std::cout << "I.E.: tms_test_app.exe 7414 true daq.opcua://168.1.10.1/" << std::endl;
        return -1;
    }

    if (!arg1.empty())
        websocketPort = stoi(arg1);

    std::string arg2 = (argc >= 3) ? argv[2] : "";
    useRenderer = false;
    if (!arg2.empty())
        useRenderer = arg2 == "true";

    std::string arg3 = (argc >= 4) ? argv[3] : "any";
    if (!arg3.empty())
        connectionString = arg3;

    return 0;
}

int main(int argc, const char* argv[])
{
    bool useRenderer;
    std::string connectionString;

    // Create an Instance
    auto instance = daq::Instance(daq::String(MODULE_PATH));
    auto logger = instance.getContext().getLogger();
    logger.setLevel(daq::LogLevel::Critical);

    auto serverTypes = instance.getAvailableServerTypes();
    auto config = serverTypes.get("openDAQ LT Streaming").createDefaultConfig();

    bool readTime = false;
    for (int i = 0; i < argc; ++i)
    {
        std::string_view arg = argv[i];
        size_t index = arg.find("readTime=");
        if (index != std::string_view::npos)
        {
            arg.remove_prefix(sizeof("readTime=") - 1);
            if (arg == "true")
            {
                readTime = true;
            }
        }
    }

    // Read input parameters
    daq::Int websocketPort = config.getPropertyValue("WebsocketStreamingPort");
    if (parseArgs(argc, argv, websocketPort, useRenderer, connectionString) < 0)
        return 0;

    config.setPropertyValue("WebsocketStreamingPort", websocketPort);

    // Start a web-socket streaming server
    instance.addServer("openDAQ LT Streaming", config);
    // Start an OpcUa server
    instance.addServer("openDAQ OpcUa", nullptr);

    // Discover and connect to devices
    auto deviceInfoList = instance.getAvailableDevices();
    std::vector<daq::DevicePtr> devices;
    std::cout << "Discovered devices: " << std::endl;

    for (const auto& deviceInfo : deviceInfoList)
        std::cout << "Name: " << deviceInfo.getName() << ", Connection string: " << deviceInfo.getConnectionString() << std::endl;

    for (const auto& deviceInfo : deviceInfoList)
    {
        if (connectionString == "any" || deviceInfo.getConnectionString() == connectionString)
        {
            daq::DevicePtr device = instance.addDevice(deviceInfo.getConnectionString());
            devices.push_back(device);
        }
    }

    daq::FunctionBlockPtr renderer;
    if (useRenderer)
        renderer = instance.addFunctionBlock("RefFBModuleRenderer");

    // Create readers and writers
    std::vector<RecordedDevicePtr> recordedDevices;
    for (auto& device : devices)
    {
        std::vector<Recorder> recorders;
        std::string deviceName = "device_" + device.getInfo().getName();
        auto signals = device.getSignals(daq::search::Recursive(daq::search::Visible()));

        std::cout << deviceName << std::endl;

        for (auto signal : signals)
        {
            if (signal.getDomainSignal().assigned())
            {
                if (useRenderer)
                {
                    auto ports = renderer.getInputPorts();
                    auto port = ports[ports.getCount() - 1];
                    port.connect(signal);
                }

                std::cout << signal.getDescriptor().getName() << std::endl;
                std::cout << signal.getGlobalId() << std::endl;
                recorders.emplace_back(signal, 1000, readTime);
            }
        }
        std::cout << std::endl;

        RecordedDevicePtr recordedDevice = std::make_shared<RecordedDevice>(deviceName, std::move(recorders));
        recordedDevice->writer->writeHeader();
        recordedDevices.push_back(recordedDevice);
    }

    // Loop procedure to read values from signals
    std::atomic stop = false;
    auto loop = [&recordedDevices, &stop]() {
        while (!stop)
        {
            for (auto& recordedDevice : recordedDevices)
            {
                for (auto& recorder : recordedDevice->recorders)
                {
                    recorder.readSamples();
                }

                recordedDevice->writer->writeBuffers();
            }
            std::this_thread::sleep_for(20ms);
        }
    };

    std::thread readLoop(loop);

    std::cout << "Recording.... Type q to stop." << std::endl;
    char q;
    std::cin >> q;

    stop = true;
    readLoop.join();

    return 0;
}
