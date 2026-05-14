/**
 * Part of the openDAQ stand-alone application quick start guide. The full
 * example can be found in app_quick_start_full.cpp
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <opendaq/opendaq.h>

#include <daq_discovery/daq_discovery_client.h>

int main(int /*argc*/, const char* /*argv*/[])
{
    for (size_t i = 0; i < 100000; ++i)
    {
        auto mdnsClient =
            std::make_shared<daq::discovery::MDNSDiscoveryClient>(
                daq::List<daq::IString>(
                    "_streaming-lt._tcp.local.",
                    "_streaming-ws._tcp.local.",
                    "_opendaq-streaming-native._tcp.local.",
                    "_opcua-tcp._tcp.local.",
                    "_opendaq-ip-modification._udp.local."
                )
            );
        mdnsClient->sendDiscoveryQueryIgnoreReplies();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
} 
