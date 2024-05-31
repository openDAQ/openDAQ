#include <opendaq/opendaq.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

using namespace daq;
using namespace std::chrono_literals;

int main(int /*argc*/, const char* /*argv*/[])
{
    // Set up the simulator

    // Create Instance
    auto instance = Instance("");
    // Set Root Device
    instance.setRootDevice("daqref://device1");
    // Add Trigger Reference Functioin Block
    instance.addFunctionBlock("ref_fb_module_trigger");
    // Add all current servers
    instance.addServer("openDAQ OpcUa", nullptr);
    instance.addServer("openDAQ Native Streaming", nullptr);
    instance.addServer("openDAQ LT Streaming", nullptr);
    // Add custom String Property
    auto stringProperty = StringPropertyBuilder("TestString", "TestDefaultString").setDescription("TestDescription").build();
    instance.addProperty(stringProperty);
    // Add custom Dictionary Property
    auto dict = Dict<StringPtr, StringPtr>();
    dict.set("TestKey", "TestValue");
    instance.addProperty(DictProperty("TestDict", dict, true));
    // Add custom Int Property
    auto list = List<IInteger>();
    list.pushBack(1);
    list.pushBack(2);
    list.pushBack(3);
    instance.addProperty(IntPropertyBuilder("TestInt", 42)
                             .setUnit(Unit("TestUnit", -1, "TestName", "TestQunatity"))
                             .setMinValue(-666)
                             .setMaxValue(777)
                             .setSuggestedValues(list)
                             .setValidator(Validator("Value < 800"))
                             .setCoercer(Coercer("if(Value > 900, Value, 900)"))
                             .build());
    // Add custom Selection Property
    instance.addProperty(SelectionProperty("TestSelection", list, 2, true));
    // Add custom Reference Property
    instance.addProperty(ReferenceProperty("TestReference", EvalValue("%TestString")));
    // Add custom Struct Property
    instance.addProperty(StructProperty("TestStruct", Struct("TestName", dict, TypeManager())));
    // TODO: FIX! Add custom getOnPropertyValueRead Event to TestInt Property
    instance.addProperty(IntProperty("IntReadCount", 0));
    instance.getOnPropertyValueRead("TestInt") += [](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
    {
        IntegerPtr count = sender.getPropertyValue("IntReadCount");
        sender.setPropertyValue("IntReadCount", count + 1);
    };
    // TODO: FIX! Add custom getOnPropertyValueWrite Event to TestInt Property
    instance.addProperty(IntProperty("IntWriteCount", 0));
    instance.getOnPropertyValueWrite("TestInt") += [](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
    {
        IntegerPtr count = sender.getPropertyValue("IntWriteCount");
        sender.setPropertyValue("IntWriteCount", count + 1);
    };

    // Create an empty file named "ready" to let regression test suite know
    // the simulator is up and running and ready for tests
    std::ofstream ready;
    ready.open("ready", std::ios::out);
    ready.close();

    // Github Action will delete the "ready" file after
    // the tests for one protocol are done, which means
    // we can then gracefully shut down the simulator
    while (std::filesystem::exists("ready"))
    {
        std::this_thread::sleep_for(100ms);
    }

    return 0;
}
