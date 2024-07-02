#include "app_input_port.h"
#include <csignal>
#include <iomanip>
#include <iostream>
#include <opendaq/search_filter_factory.h>

BEGIN_NAMESPACE_OPENDAQ

bool AppInputPort::processCommand(BaseObjectPtr& port, const std::vector<std::string>& command, const InstancePtr& instance)
{
    if (command.empty())
        return false;

    if (command[0] == "Connect")
        return connect(port, command, instance);
    if (command[0] == "Disconnect")
        return disconnect(port);
    if (command[0] == "Print")
        return print(port, command);
    if (command[0] == "Help")
        return help();
    if (command[0] == "Select")
        return select(port, command);

    return false;
}

bool AppInputPort::connect(const InputPortPtr& port, const std::vector<std::string>& command, const InstancePtr& instance)
{
    if (command.size() != 2)
        return false;

    std::string id = command[1];
    for (auto signal : instance.getSignalsRecursive())
    {
        if (signal.getGlobalId() == id)
        {
            port.connect(signal);
            return true;
        }
    }

    std::cout << "Invalid signal ID." << std::endl;
    return true;
}

bool AppInputPort::disconnect(const InputPortPtr& port)
{
    if (port.getSignal().assigned())
        port.disconnect();
    else
        std::cout << "No signal connected." << std::endl;

    return true;
}

bool AppInputPort::print(const InputPortPtr& port, const std::vector<std::string>& command)
{
    if (command.size() != 2)
        return false;

    if (command[1] == "Name")
    {
        const auto name = port.getName();
        if (name.assigned() && name.getLength() > 0)
            std::cout << "Name: " + name << std::endl;
        else
            std::cout << "Not available." << std::endl;

        return true;
    }

    if (command[1] == "requires-signal")
    {
        if (port.getRequiresSignal())
            std::cout << "Requires signal: True" << std::endl;
        else
            std::cout << "Requires signal: False" << std::endl;

        return true;
    }

    if (command[1] == "signal-id")
    {
        const auto signal = port.getSignal();
        if (!signal.assigned())
        {
            std::cout << "<disconnected>" << std::endl;
            return true;
        }

        const auto id = signal.getGlobalId();
        std::cout << "Signal ID: " + id << std::endl;

        return true;
    }

    return false;
}

bool AppInputPort::select(BaseObjectPtr& port, const std::vector<std::string>& command)
{
    const auto portPtr = port.asPtr<IInputPort>();
    if (command.size() == 2 && command[1] == "signal")
    {
        if (portPtr.getSignal().assigned())
            port = portPtr.getSignal();
        else
            std::cout << "No signal connected to input port." << std::endl;
        return true;
    }

    return false;
}

bool AppInputPort::help()
{
    std::cout << std::setw(25) << std::left << "connect <signal-id>"
              << "Connects the signal with <signal-id> to the input port" << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "Disconnect"
              << "Disconnects the connected signal, if a signal is connected to the input port." << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "select signal"
               << "Moves to application to the connected signal if available." << std::endl
               << std::endl;

    std::cout << std::setw(25) << std::left << "print <info>"
              << "Prints the value of the given <info> parameter. Available information:" << std::endl
              << std::setw(25) << ""
              << "[name, signal-id, requires-signal]" << std::endl
              << std::endl;

    return true;
}

END_NAMESPACE_OPENDAQ
