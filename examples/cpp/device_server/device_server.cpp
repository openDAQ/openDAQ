#include <iostream>
#include <opendaq/opendaq.h>

using namespace daq;

int main(int /*argc*/, const char* /*argv*/[])
{
    const InstancePtr instance = Instance("");
    instance.setRootDevice("daqref://device0");
    auto dummy = instance.addStandardServers();
    instance.addProperty(ListProperty("listString", List<IString>()));
    instance.addProperty(ListProperty("listBase", List<IBaseObject>()));
    instance.addProperty(ListProperty("listInt", List<IInteger>()));
    instance.addProperty(DictProperty("dict", Dict<IBaseObject, IBaseObject>()));
    instance.addProperty(DictProperty("dictString", Dict<IString, IString>()));
    std::cin.get();
    return 0;
}
