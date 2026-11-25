// DIAGNOSTIC ONLY: records the stack of every addRef/releaseRef on a TypeManager and prints the stacks whose
// references were never released when a leak is reported
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <cstdio>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
    struct RefEvent
    {
        int delta;
        USHORT frameCount;
        void* frames[40];
    };

    std::mutex& registryMutex()
    {
        static std::mutex* mutex = new std::mutex();
        return *mutex;
    }

    std::unordered_map<void*, std::vector<RefEvent>>& registry()
    {
        static auto* map = new std::unordered_map<void*, std::vector<RefEvent>>();
        return *map;
    }

    void printFrame(HANDLE process, void* address)
    {
        char buffer[sizeof(SYMBOL_INFO) + 512] = {};
        auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = 511;
        DWORD64 displacement = 0;
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(line);
        DWORD lineDisplacement = 0;
        const auto addr = reinterpret_cast<DWORD64>(address);
        const bool hasSymbol = SymFromAddr(process, addr, &displacement, symbol);
        const bool hasLine = SymGetLineFromAddr64(process, addr, &lineDisplacement, &line);
        std::printf("      %s+0x%llx  %s:%lu\n",
                    hasSymbol ? symbol->Name : "?",
                    static_cast<unsigned long long>(displacement),
                    hasLine ? line.FileName : "?",
                    hasLine ? line.LineNumber : 0ul);
    }
}

void diagTypeManagerRegister(void* object)
{
    std::lock_guard lock(registryMutex());
    registry()[object];
}

void diagTypeManagerUnregister(void* object)
{
    std::lock_guard lock(registryMutex());
    registry().erase(object);
}

void diagTypeManagerRecord(void* object, int delta)
{
    RefEvent event{delta, 0, {}};
    event.frameCount = CaptureStackBackTrace(1, 40, event.frames, nullptr);
    std::lock_guard lock(registryMutex());
    if (auto it = registry().find(object); it != registry().end())
        it->second.push_back(event);
}

void diagTypeManagerDumpAll()
{
    std::unordered_map<void*, std::vector<RefEvent>> snapshot;
    {
        std::lock_guard lock(registryMutex());
        snapshot = registry();
    }

    const HANDLE process = GetCurrentProcess();
    static const bool symbolsReady = []
    {
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        return SymInitialize(GetCurrentProcess(), nullptr, TRUE) == TRUE;
    }();

    std::printf("DIAG: %zu TypeManager(s) alive (symbols %s)\n", snapshot.size(), symbolsReady ? "ok" : "unavailable");
    for (const auto& [object, events] : snapshot)
    {
        std::map<std::vector<void*>, int> balance;
        int net = 0;
        for (const auto& event : events)
        {
            balance[std::vector<void*>(event.frames, event.frames + event.frameCount)] += event.delta;
            net += event.delta;
        }

        std::printf("DIAG: TypeManager %p: %zu ref events, net %d over virtual addRef/releaseRef\n", object, events.size(), net);
        for (const auto& [frames, count] : balance)
        {
            if (count <= 0)
                continue;
            std::printf("DIAG:   unreleased %+d from:\n", count);
            for (const auto frame : frames)
                printFrame(process, frame);
        }
    }
    std::fflush(stdout);
}
#else
void diagTypeManagerRegister(void*) {}
void diagTypeManagerUnregister(void*) {}
void diagTypeManagerRecord(void*, int) {}
void diagTypeManagerDumpAll() {}
#endif
