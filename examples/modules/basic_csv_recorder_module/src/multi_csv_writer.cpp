#include <opendaq/custom_log.h>

#include <basic_csv_recorder_module/multi_csv_writer.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

MultiCsvWriter::MultiCsvWriter(const fs::path& filename)
    : exitFlag(false)
    , writerThread([this]() { this->threadLoop(); })
{
    if (filename.has_parent_path())
    {
        fs::create_directories(filename.parent_path());
    }

    outFile.exceptions(std::ios::failbit | std::ios::badbit);
    outFile.open(filename);
}

MultiCsvWriter::~MultiCsvWriter()
{
    std::unique_lock lock(mutex);
    exitFlag = true;
    lock.unlock();
    cv.notify_all();

    writerThread.join();
}

void MultiCsvWriter::setHeaderInformation()
{
    // TODO: This should probably be done on initialization (in constructor)
}

void MultiCsvWriter::writeSamples(std::vector<std::unique_ptr<double[]>>&& jaggedArray, int count)
{
    std::unique_lock lock(mutex);
    exitFlag = false;
    queue.emplace(JaggedBuffer{count, std::move(jaggedArray)});
    lock.unlock();
    cv.notify_all();
}

void MultiCsvWriter::threadLoop()
{
    while (true)
    {
        std::unique_lock lock{mutex};
        cv.wait(lock, [this]() { return !queue.empty() || exitFlag; });

        if (exitFlag)
        {
            return;
        }

        JaggedBuffer samples = std::move(queue.front());
        queue.pop();
        lock.unlock();

        const size_t signalNum = samples.buffer.size();
        // Write to file
        for (size_t i = 0; i < samples.count; ++i)
        {
            for (size_t signal = 0; signal < signalNum; ++signal)
            {
                double* signalSamplesPtr = samples.buffer[signal].get();
                outFile << signalSamplesPtr[i];

                if (signal != signalNum - 1)
                {
                    outFile << ",";
                }
            }
            outFile << "\n";
        }
    }
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE