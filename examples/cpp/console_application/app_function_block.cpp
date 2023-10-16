#include "app_function_block.h"
#include "app_property_object.h"
#include <iostream>
#include <iomanip>

BEGIN_NAMESPACE_OPENDAQ

bool AppFunctionBlock::processCommand(BaseObjectPtr& fb, const std::vector<std::string>& command)
{
    if (command.empty())
        return false;

    if (command[0] == "list")
        return list(fb, command);
    if (command[0] == "select")
        return select(fb, command);
    if (command[0] == "print")
        return print(fb, command);
    if (command[0] == "help")
        return help();

    return AppPropertyObject::processCommand(fb, command);
}

bool AppFunctionBlock::list(const FunctionBlockPtr& fb, const std::vector<std::string>& command)
{
    if (command.size() != 2)
        return false;

    if (command[1] == "signals")
    {
        int cnt = 0;
        for (auto signal : fb.getSignals())
        {
            auto descriptor = signal.getDescriptor();
            if (!descriptor.assigned())
                continue;

            std::string name = signal.getDescriptor().getName().assigned() ? signal.getDescriptor().getName() : "";
            std::string id = signal.getGlobalId();
            std::cout << "[" << std::to_string(cnt) << "] Name: " << name << ", Unique ID: " << id << std::endl;
            cnt++;
        }
        return true;
    }

    if (command[1] == "input-ports")
    {
        int cnt = 0;
        for (auto port : fb.getInputPorts())
        {
            std::string name = port.getName().assigned() ? port.getName() : "";
            std::string signalId = port.getSignal().assigned() ? port.getSignal().getGlobalId() : "<disconnected>";
            std::cout << "[" << std::to_string(cnt) << "] Name: " << name << ", Signal ID: " << signalId << std::endl;
            cnt++;
        }
        return true;
    }

    return AppPropertyObject::list(fb, command);
}

bool AppFunctionBlock::select(BaseObjectPtr& fb, const std::vector<std::string>& command)
{
    const auto fbPtr = fb.asPtr<IFunctionBlock>();

    if ((command.size() == 2 || command.size() == 3) && command[1] == "status-signal")
    {
        if (fbPtr.getStatusSignal().assigned())
            fb = fbPtr.getStatusSignal();
        else
            std::cout << "Status signal not available." << std::endl;
        return true;
    }

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

    if (command[1] == "signal")
    {
        if (fbPtr.getSignals().getCount() > index)
            fb = fbPtr.getSignals()[index];
        else
            std::cout << "Index out of bounds." << std::endl;
        return true;
    }

    if (command[1] == "input-port")
    {
        if (fbPtr.getInputPorts().getCount() > index)
            fb = fbPtr.getInputPorts()[index];
        else
            std::cout << "Index out of bounds." << std::endl;
        return true;
    }

    return false;
}

bool AppFunctionBlock::print(const FunctionBlockPtr& fb, const std::vector<std::string>& command)
{
    if (command.size() != 2)
        return false;

    if (command[1] == "name")
    {
        const auto name = fb.getFunctionBlockType().getName();
        if (name.assigned() && name.getLength() > 0)
            std::cout << "Name: " + name << std::endl;
        else
            std::cout << "Not available." << std::endl;

        return true;
    }

    if (command[1] == "id")
    {
        const auto id = fb.getFunctionBlockType().getId();
        if (id.assigned() && id.getLength() > 0)
            std::cout << "ID: " + id << std::endl;
        else
            std::cout << "Not available." << std::endl;

        return true;
    }

    if (command[1] == "description")
    {
        const auto desc = fb.getFunctionBlockType().getDescription();
        if (desc.assigned() && desc.getLength() > 0)
            std::cout << "Description: " + desc << std::endl;
        else
            std::cout << "Not available." << std::endl;

        return true;
    }

    return false;
}

bool AppFunctionBlock::help()
{
    if (!AppPropertyObject::help())
        return false;
    
    std::cout << std::setw(25) << std::left << "list <type>"
              << "Lists added all child objects of chosen <type>. Available types:" << std::endl
              << std::setw(25) << ""
              << "[signals, input-ports]." << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "select <type> <index>"
              << "Moves the application to a child of chosen <type> at a given <index>." << std::endl
              << std::setw(25) << ""
              << "Available types: [signal, input-port, status-signal]." << std::endl
              << std::setw(25) << ""
              << "The index is ignored if the status-signal is chosen for the <type> parameter" << std::endl
              << std::setw(25) << ""
              << "and can be omitted." << std::endl
              << std::endl;

    std::cout << std::setw(25) << std::left << "print <info>"
              << "Prints the value of the given <info> parameter. Available information:" << std::endl
              << std::setw(25) << ""
              << "[name, id, description]" << std::endl
              << std::endl;

    return true;
}

END_NAMESPACE_OPENDAQ
