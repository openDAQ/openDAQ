#include <iostream>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char** /*argv*/)
{
    auto instance = Instance();
    instance.addDevice("opc.tcp://127.0.0.1:4840");

    std::cout << "Type q to quit..." << std::endl;
    std::string input;
    std::cin >> input;

    return 0;
}
