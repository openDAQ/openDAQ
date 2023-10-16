/**
 * Part of the openDAQ stand-alone application quick start guide. The full
 * example can be found in app_quick_start_full.cpp
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <opendaq/opendaq.h>

int main(int /*argc*/, const char* /*argv*/[])
{
    // Create an Instance, loading modules at MODULE_PATH
    const daq::InstancePtr instance = daq::Instance(MODULE_PATH);

    std::cout << "Press \"enter\" to exit the application..." << std::endl;
    std::cin.get();
    return 0;
}
