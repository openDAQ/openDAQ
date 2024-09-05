#include "console_application.h"
#include "app_device.h"
#include "app_signal.h"
#include "app_channel.h"
#include "app_function_block.h"
#include "app_input_port.h"
#include <iostream>
#include <iomanip>

BEGIN_NAMESPACE_OPENDAQ

ConsoleApplication::ConsoleApplication(const InstancePtr& instance)
    : instance(instance)
{
    currentObject = instance;
}

void ConsoleApplication::start()
{
    bool exit = false;
    while (!exit)
    {
        std::cout << getPathString() << " > ";
        std::string input;
        std::getline(std::cin, input);

        std::string s;
        std::stringstream ss(input);
        std::vector<std::string> command;
        while (std::getline(ss, s, ' '))
            command.push_back(s);

        exit = processCommand(command);
    }
}

std::string ConsoleApplication::getTypeString(const BaseObjectPtr& obj)
{
    if (obj.supportsInterface<IInstance>())
        return "instance";
    if (obj.supportsInterface<IDevice>())
        return "device";
    if (obj.supportsInterface<IChannel>())
        return "channel";
    if (obj.supportsInterface<IFunctionBlock>())
        return "function-block";
    if (obj.supportsInterface<ISignal>())
        return "signal";
    if (obj.supportsInterface<IInputPort>())
        return "input-port";
    return "";
}

std::string ConsoleApplication::getPathString()
{
    std::string path = "";
    for (const auto& parent : parents)
        path += getTypeString(parent) + "/";
    path += getTypeString(currentObject);
    return path;
}

bool ConsoleApplication::processCommand(const std::vector<std::string>& command)
{
    if (!command.empty() && command[0] == "exit")
        return true;

    if (!command.empty() && command[0] == "Help")
    {
        std::cout << std::setw(25) << std::left << "exit"
                  << "Exits the application." << std::endl
                  << std::endl;

        std::cout << std::setw(25) << std::left << "/"
                  << "Moves the application to the root device." << std::endl
                  << std::endl;

        std::cout << std::setw(25) << std::left << ".."
                  << "Moves the application to the previously selected object." << std::endl
                  << std::endl;
    }

    if (!command.empty() && command[0] == "..")
    {

        if (!parents.empty())
        {
            currentObject = parents.back();
            parents.pop_back();
        }
        else
        {
            std::cout << "Currently accessing root device." << std::endl;
        }
        return false;
    }

    if (!command.empty() && command[0] == "/")
    {
        parents.clear();
        currentObject = instance;
        return false;
    }

    const BaseObjectPtr prev = currentObject;

    bool validCommand = false;
    if (currentObject.supportsInterface<IDevice>())
        validCommand = AppDevice::processCommand(currentObject, command);
    else if (currentObject.supportsInterface<IChannel>())
        validCommand = AppChannel::processCommand(currentObject, command);
    else if (currentObject.supportsInterface<IFunctionBlock>())
        validCommand = AppFunctionBlock::processCommand(currentObject, command);
    else if (currentObject.supportsInterface<ISignal>())
        validCommand = AppSignal::processCommand(currentObject, command);
    else if (currentObject.supportsInterface<IInputPort>())
        validCommand = AppInputPort::processCommand(currentObject, command, instance);

    if (!validCommand)
        std::cout << "Invalid command. Type help for a list of available commands." << std::endl;

    if (prev != currentObject)
        parents.push_back(prev);
    
    return false;
}

END_NAMESPACE_OPENDAQ
