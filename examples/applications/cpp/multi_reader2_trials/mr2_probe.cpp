/**
 * Discovers the openDAQ devices on the network, connects to each one that is not a local module device, and writes
 * what the trials need to know about it: identity, server protocols, sub-devices, and the domain of every signal.
 * Changes nothing on the devices.
 */

#include "trial_common.h"

#include <iostream>

using namespace daq;

static std::string domainSummary(const SignalPtr& signal)
{
    const auto domainSignal = signal.getDomainSignal();
    const auto d = domainSignal.assigned() ? domainSignal.getDescriptor() : nullptr;
    if (!d.assigned())
        return "no domain";
    const auto rule = d.getRule();
    std::string ruleText = "none";
    if (rule.assigned())
    {
        ruleText.clear();
        for (const auto& [key, value] : rule.getParameters())
            ruleText += fmt::format("{}={} ", key, value);
    }
    const auto info = d.getReferenceDomainInfo();
    const auto res = d.getTickResolution();
    return fmt::format("rule {} {}, res {}/{}, origin {}, ref {} off {}",
                       rule.assigned() ? static_cast<int>(rule.getType()) : -1,
                       ruleText,
                       res.assigned() ? res.getNumerator() : 0,
                       res.assigned() ? res.getDenominator() : 0,
                       d.getOrigin(),
                       info.assigned() ? info.getReferenceDomainId() : "-",
                       info.assigned() && info.getReferenceDomainOffset().assigned() ? info.getReferenceDomainOffset() : 0);
}

static void printProperties(std::ostream& out, const PropertyObjectPtr& obj, const std::string& indent)
{
    for (const auto& p : obj.getVisibleProperties())
    {
        if (p.getValueType() == ctObject)
            continue;
        std::string value;
        try
        {
            value = fmt::format("{}", obj.getPropertyValue(p.getName()));
            if (p.getSelectionValues().assigned())
                value += fmt::format(" ({})", obj.getPropertySelectionValue(p.getName()));
        }
        catch (const std::exception& e)
        {
            value = std::string("<") + e.what() + ">";
        }
        out << indent << p.getName() << " = " << value << (p.getReadOnly() ? " [ro]" : "") << "\n";
    }
}

static void printDevice(std::ostream& out, const DevicePtr& device, const std::string& indent)
{
    const auto info = device.getInfo();
    out << indent << "device " << device.getGlobalId() << " name=" << info.getName() << " serial=" << info.getSerialNumber()
        << " model=" << info.getModel() << " manufacturer=" << info.getManufacturer() << " cs=" << info.getConnectionString() << "\n";
    printProperties(out, device, indent + "  prop ");
    for (const auto& ch : device.getChannels())
    {
        out << indent << "  channel " << ch.getGlobalId() << "\n";
        printProperties(out, ch, indent + "    prop ");
    }
    for (const auto& s : device.getSignals(search::Recursive(search::Visible())))
    {
        const auto d = s.getDescriptor();
        out << indent << "  signal " << s.getGlobalId() << " type=" << (d.assigned() ? static_cast<int>(d.getSampleType()) : -1)
            << " unit=" << trial::unitOf(d) << " | " << domainSummary(s) << "\n";
    }
    for (const auto& sub : device.getDevices())
        printDevice(out, sub, indent + "  ");
}

int main(int argc, const char* argv[])
{
    const auto dir = trial::outputDir("probe");
    std::ofstream out(dir / "probe.txt");
    const InstancePtr instance = Instance(MODULE_PATH);

    const auto infos = instance.getAvailableDevices();
    for (const auto& info : infos)
    {
        out << "discovered " << info.getConnectionString() << " name=" << info.getName() << " serial=" << info.getSerialNumber()
            << " model=" << info.getModel() << " manufacturer=" << info.getManufacturer() << "\n";
        for (const auto& cap : info.getServerCapabilities())
            out << "  server " << cap.getProtocolId() << " " << cap.getConnectionString() << " version " << cap.getProtocolVersion() << "\n";
    }
    out.flush();

    // argv: optional connection strings to open; default: every discovered daq.nd:// device
    std::vector<std::string> targets;
    for (int i = 1; i < argc; ++i)
        targets.emplace_back(argv[i]);
    if (targets.empty())
        for (const auto& info : infos)
            if (info.getConnectionString().toStdString().rfind("daq://", 0) == 0)
                targets.push_back(info.getConnectionString());

    for (const auto& cs : targets)
    {
        out << "\n=== " << cs << "\n";
        // The raw call keeps the whole error chain, which the exception reduces to its last entry
        IDevice* raw = nullptr;
        const ErrCode err = instance->addDevice(&raw, String(cs), nullptr);
        if (OPENDAQ_FAILED(err))
        {
            out << fmt::format("connect failed: 0x{:08X}\n", static_cast<uint32_t>(err));
            IList* list = nullptr;
            daqGetErrorInfoList(&list);
            for (const auto& item : ListPtr<IBaseObject>::Adopt(list))
            {
                IString* message = nullptr;
                item.asPtr<IErrorInfo>()->getFormattedMessage(&message);
                out << "  cause: " << StringPtr::Adopt(message) << "\n";
            }
            daqClearErrorInfo();

            // The module manager keeps only its own message; asking each module directly shows the module's
            for (const auto& module : instance.getModuleManager().getModules())
            {
                try
                {
                    if (module.createDevice(cs, instance.getRootDevice()).assigned())
                        out << "  module " << module.getModuleInfo().getName() << " created it directly\n";
                }
                catch (const NotFoundException&)
                {
                }
                catch (const DaqException& e)
                {
                    out << "  module " << module.getModuleInfo().getName() << ": " << e.what() << "\n";
                }
            }
        }
        else
        {
            const auto device = DevicePtr::Adopt(raw);
            printDevice(out, device, "");
            instance.removeDevice(device);
        }
        out.flush();
    }
    fmt::println("probe written to {}", (dir / "probe.txt").string());
    return 0;
}
