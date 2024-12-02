#include "app_device.h"
#include <iomanip>
#include <iostream>
#include "app_property_object.h"

BEGIN_NAMESPACE_OPENDAQ

bool AppDevice::processCommand(BaseObjectPtr& device, const std::vector<std::string>& command)
{
    if (command.empty())
        return false;

    if (command[0] == "list")
        return list(device, command);
    if (command[0] == "list-available")
        return listAvailable(device, command);
    if (command[0] == "add")
        return add(device, command);
    if (command[0] == "remove")
        return remove(device, command);
    if (command[0] == "select")
        return select(device, command);
    if (command[0] == "print")
        return print(device, command);
    if (command[0] == "help")
        return help();

    return AppPropertyObject::processCommand(device, command);
}

bool AppDevice::list(const DevicePtr& device, const std::vector<std::string>& command)
{
    if (command.size() != 2)
        return false;

    if (command[1] == "devices")
    {
        int cnt = 0;
        for (auto subDevice : device.getDevices())
        {
            std::string name = subDevice.getInfo().getName().assigned() ? subDevice.getInfo().getName() : "";
            std::string serialNumber = subDevice.getInfo().getSerialNumber().assigned() ? subDevice.getInfo().getSerialNumber() : "";
            std::string connectionString = subDevice.getInfo().getConnectionString().assigned() ? subDevice.getInfo().getConnectionString() : "";
            std::cout << "[" << std::to_string(cnt) << "] Name: " << name << ", Serial number: " << serialNumber
                      << ", Connection string: " << connectionString
                      << std::endl;
            cnt++;
        }
        return true;
    }

    if (command[1] == "channels")
    {
        int cnt = 0;
        for (auto channel : device.getChannels())
        {
            std::string name = channel.getFunctionBlockType().getName().assigned() ? channel.getFunctionBlockType().getName() : "";
            std::string id = channel.getFunctionBlockType().getId().assigned() ? channel.getFunctionBlockType().getId() : "";
            std::cout << "[" << std::to_string(cnt) << "] Name: " << name << ", Unique ID: " << id << std::endl;
            cnt++;
        }
        return true;
    }
    
    if (command[1] == "function-blocks")
    {
        int cnt = 0;
        for (auto fb : device.getFunctionBlocks())
        {
            std::string name = fb.getFunctionBlockType().getName().assigned() ? fb.getFunctionBlockType().getName() : "";
            std::string id = fb.getFunctionBlockType().getId().assigned() ? fb.getFunctionBlockType().getId() : "";
            std::cout << "[" << std::to_string(cnt) << "] Name: " << name << ", Unique ID: " << id << std::endl;
            cnt++;
        }
        return true;
    }
    
    if (command[1] == "signals")
    {
        int cnt = 0;
        for (auto signal : device.getSignals())
        {
            std::string name = signal.getDescriptor().getName().assigned() ? signal.getDescriptor().getName() : "";
            std::string id = signal.getGlobalId();
            std::cout << "[" << std::to_string(cnt) << "] Name: " << name << ", Unique ID: " << id << std::endl;
            cnt++;
        }
        return true;
    }

    return AppPropertyObject::list(device, command);
}

bool AppDevice::select(BaseObjectPtr& device, const std::vector<std::string>& command)
{
    if (command.size() != 3)
        return false;

    size_t index;
    try
    {
        index = std::stoi(command[2]);
    }
    catch (...)
    {
        return false;
    }

    const auto devicePtr = device.asPtr<IDevice>();
    if (command[1] == "device")
    {
        if (devicePtr.getDevices().getCount() > index)
            device = devicePtr.getDevices()[index];
        else
            std::cout << "Index out of bounds." << std::endl;
        return true;
    }

    if (command[1] == "function-block")
    {
        if (devicePtr.getFunctionBlocks().getCount() > index)
            device = devicePtr.getFunctionBlocks()[index];
        else
            std::cout << "Index out of bounds." << std::endl;
        return true;
    }

    if (command[1] == "channel")
    {
        if (devicePtr.getChannels().getCount() > index)
            device = devicePtr.getChannels()[index];
        else
            std::cout << "Index out of bounds." << std::endl;
        return true;
    }

    if (command[1] == "signal")
    {
        if (devicePtr.getSignals().getCount() > index)
            device = devicePtr.getSignals()[index];
        else
            std::cout << "Index out of bounds." << std::endl;
        return true;
    }

    return false;
}

bool AppDevice::listAvailable(const DevicePtr& device, const std::vector<std::string>& command)
{
    if (command.size() != 2)
        return false;

    if (command[1] == "devices")
    {
        int cnt = 0;
        for (const auto& discoveredDevice : device.getAvailableDevices())
        {
            std::string name = discoveredDevice.getName().assigned() ? discoveredDevice.getName() : "";
            std::string serialNumber = discoveredDevice.getSerialNumber().assigned() ? discoveredDevice.getSerialNumber() : "";
            std::cout << "[" << std::to_string(cnt) << "] Name: " << name << ", Serial number: " << serialNumber
                      << ", Connection string: " << discoveredDevice.getConnectionString()
                      << std::endl;
            cnt++;
        }
        return true;
    }

    if (command[1] == "function-blocks")
    {
        int cnt = 0;
        for (const auto& [_, fb] : device.getAvailableFunctionBlockTypes())
        {
            std::string name = fb.getName().assigned() ? fb.getName() : "";
            std::string id = fb.getId().assigned() ? fb.getId() : "";
            std::cout << "[" << std::to_string(cnt) << "] Name: " << name << ", Unique ID: " << id << std::endl;
            cnt++;
        }
        return true;
    }

    return false;
}

bool AppDevice::add(const DevicePtr& device, const std::vector<std::string>& command)
{
    if (command.size() != 3)
        return false;

    if (command[1] == "device")
    {
        try
        {
            device.addDevice(command[2]);
            return true;
        }
        catch (...)
        {
            std::cout << "Unable to add device with the given connection string" << std::endl;
            return true;
        }
    }

    if (command[1] == "function-block")
    {
        try
        {
            device.addFunctionBlock(command[2]);
            return true;
        }
        catch (...)
        {
            std::cout << "Unable to add function block with the given ID" << std::endl;
            return true;
        }
    }

    return false;
}

bool AppDevice::remove(const DevicePtr& device, const std::vector<std::string>& command)
{
    if (command.size() != 3)
        return false;

    size_t index;
    try
    {
        index = std::stoi(command[2]);
    }
    catch (...)
    {
        return false;
    }

    if (command[1] == "device")
    {
        if (device.getDevices().getCount() > index)
            device.removeDevice(device.getDevices()[index]);
        else
            std::cout << "Index out of bounds." << std::endl;
        return true;
    }
    
    if (command[1] == "function-block")
    {
        if (device.getFunctionBlocks().getCount() > index)
            device.removeFunctionBlock(device.getFunctionBlocks()[index]);
        else
            std::cout << "Index out of bounds." << std::endl;
        return true;
    }

    return false;
}

bool AppDevice::print(const DevicePtr& device, const std::vector<std::string>& command)
{
    if (command.size() != 2)
        return false;

    const auto propName = command[1];
    const auto info = device.getInfo();

    if (propName == "all")
    {
        for (const auto prop : info.getAllProperties())
        {
            const auto name = prop.getName();
            const auto value = info.getPropertyValue(name);
            std::cout << name << ": " << value << std::endl;
        }

        return true;
    }

    if (info.hasProperty(propName))
    {
        std::cout << propName << ": " << info.getPropertyValue(propName) << std::endl;
        return true;
    }

    std::cout << "Not available." << std::endl;
    return false;
}

bool AppDevice::help()
{
    if (!AppPropertyObject::help())
        return false;
    
    std::cout << std::setw(25) << std::left << "list <type>"
              << "Lists added all child objects of chosen <type>. Available types:" << std::endl
              << std::setw(25) << ""
              << "[devices, channels, function-blocks, signals]." << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "select <type> <index>"
              << "Moves the application to a child of chosen <type> at a given <index>." << std::endl
              << std::setw(25) << ""
              << "Available types: [device, function-block]." << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "list-available <type>"
              << "Lists all objects of chosen <type> that can be added via `add`." << std::endl
              << std::setw(25) << ""
              << "Available types: [devices, function-blocks]." << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "add <type> <address/id>"
              << "Adds an object of <type> at the given address, or with the given ID. " << std::endl
              << std::setw(25) << ""
              << "Available types: [device, function-block]. To add function blocks," << std::endl
              << std::setw(25) << ""
              << "enter their ID. To add devices, enter their connection string." << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "remove <type> <index>" 
              << "Remove an object of given <type> at the given <index>." << std::endl
              << std::setw(25) << ""
              << "Available types: [device, function-block]. The index can be obtained" << std::endl
              << std::setw(25) << ""
              << "by listing added devices or function blocks via `list`." << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "print <info>"
              << "Prints the value of the given <info> parameter. Available information:" << std::endl
              << std::setw(25) << ""
              << "[name, location, model, serial-number, locking-context, platform," << std::endl
              << std::setw(25) << ""
              << "firmware-version, connection-string, metadata]." << std::endl
              << std::endl;

    return true;
}

END_NAMESPACE_OPENDAQ
