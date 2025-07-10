#include <exception>
#include <memory>
#include <mutex>

#include <coretypes/filesystem.h>
#include <opendaq/custom_log.h>
#include <opendaq/event_packet_params.h>
#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/basic_csv_recorder_thread.h>
#include <basic_csv_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

BasicCsvRecorderThread::BasicCsvRecorderThread(
        fs::path path,
        const std::string& format,
        const SignalPtr& signal,
        const LoggerComponentPtr& loggerComponent)
    : loggerComponent(loggerComponent)
    , recorderSignal(std::make_unique<BasicCsvRecorderSignal>(path, format, signal))
    , thread([this]() { threadMain(); })
{
}

BasicCsvRecorderThread::~BasicCsvRecorderThread()
{
    post(nullptr);
    thread.join();
}

void BasicCsvRecorderThread::post(const PacketPtr& packet)
{
    std::unique_lock lock(mutex);
    queue.emplace(packet);
    lock.unlock();
    cv.notify_all();
}

void BasicCsvRecorderThread::threadMain()
{
    while (true)
    {
        std::unique_lock lock(mutex);
        cv.wait(lock, [this]() { return !queue.empty(); });
        auto packet = queue.front();
        queue.pop();
        lock.unlock();

        if (!packet.assigned())
            return;

        if (!recorderSignal)
            return;

        try
        {
            recorderSignal->onPacketReceived(packet);
        }

        catch (const std::exception& ex)
        {
            LOG_E("BasicCsvRecorder failed to write packet; closing CSV file: {}", ex.what());
            recorderSignal.reset();
        }
    }
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
