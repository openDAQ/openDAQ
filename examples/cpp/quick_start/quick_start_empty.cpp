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

    auto device = instance.addDevice("daqref://device0");

    auto modes = device.getAvailableOperationModes();
    std::cout << "Available operation modes:\n";
    for (const auto& mode : modes) {
        std::cout << "  " << mode << "\n";
    }

    auto oldMode = device.getOperationMode();
    device.setOperationMode(daq::OperationModeType::Idle);
    auto newMode = device.getOperationMode();


    std::cout << "Press \"enter\" to exit the application...\n";
    std::cin.get();
    return 0;
} 
