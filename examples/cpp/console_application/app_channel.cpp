#include "app_channel.h"
#include "app_function_block.h"
#include <iostream>
#include <iomanip>

BEGIN_NAMESPACE_OPENDAQ

bool AppChannel::processCommand(BaseObjectPtr& channel, const std::vector<std::string>& command)
{
    if (command.empty())
        return false;

    if (command[0] == "Print")
        return print(channel, command);
    if (command[0] == "Help")
        return help();

    return AppFunctionBlock::processCommand(channel, command);
}

bool AppChannel::print(const ChannelPtr& channel, const std::vector<std::string>& command)
{
    if (command.size() == 2 && command[1] == "Tags")
    {
        std::cout << "[";
        const auto tags = channel.getTags().getList();
        for (auto it = tags.begin(); it != tags.end(); ++it)
        {
            std::cout << *it;
            if (std::next(it) != tags.begin())
                std::cout << ",";
        }
        std::cout << "]" << std::endl;

        return true;
    }

    return AppFunctionBlock::print(channel, command);
}

bool AppChannel::help()
{
    if (!AppFunctionBlock::help())
        return false;
    
    std::cout << std::setw(25) << std::left << "print tags"
              << "Prints the channel's tags." << std::endl
              << std::endl;

    return true;
}

END_NAMESPACE_OPENDAQ
