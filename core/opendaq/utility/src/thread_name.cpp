#include <opendaq/thread_name.h>
#include <opendaq/utils/thread_name.h>

extern "C" PUBLIC_EXPORT void daqNameThread(const char* name)
{
    daq::utils::setThreadName(name);
}
