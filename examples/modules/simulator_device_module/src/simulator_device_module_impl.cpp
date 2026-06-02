#include <coretypes/version_info_factory.h>
#include <opendaq/custom_log.h>
#include <simulator_device_module/simulator_device_impl.h>
#include <simulator_device_module/simulator_device_module_impl.h>
#include <simulator_device_module/version.h>
#include <boost/asio/ip/host_name.hpp>

#ifdef _WIN32
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <unistd.h>
#endif

// Put this as a static helper in simulator_device_impl.cpp or a shared utils header
static std::string getMacAddress()
{
#ifdef _WIN32
    ULONG bufLen = 0;
    GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &bufLen);
    
    std::vector<uint8_t> buf(bufLen);
    auto* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
    
    if (GetAdaptersAddresses(AF_INET, 0, nullptr, adapters, &bufLen) != NO_ERROR)
        return "00:00:00:00:00:00";
    
    // Find the first adapter with a real MAC (skip loopback, tunnel, etc.)
    for (auto* a = adapters; a != nullptr; a = a->Next)
    {
        if (a->PhysicalAddressLength == 6 && a->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
        {
            return fmt::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                a->PhysicalAddress[0], a->PhysicalAddress[1],
                a->PhysicalAddress[2], a->PhysicalAddress[3],
                a->PhysicalAddress[4], a->PhysicalAddress[5]);
        }
    }
    return "00:00:00:00:00:00";
#else
    struct ifaddrs* iflist;
    if (getifaddrs(&iflist) != 0)
        return "00:00:00:00:00:00";
    
    std::string result = "00:00:00:00:00:00";
    for (auto* ifa = iflist; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_PACKET)
            continue;
        if (ifa->ifa_flags & IFF_LOOPBACK)
            continue;
            
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct ifreq ifr = {};
        strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ - 1);
        
        if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0)
        {
            auto* mac = reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
            result = fmt::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            close(fd);
            break;
        }
        close(fd);
    }
    freeifaddrs(iflist);
    return result;
#endif
}

BEGIN_NAMESPACE_SIMULATOR_DEVICE_MODULE

SimulatorDeviceModule::SimulatorDeviceModule(const ContextPtr& context)
    : Module(
          SIMULATOR_MODULE_NAME,
          VersionInfo(SIMULATOR_DEVICE_MODULE_MAJOR_VERSION, SIMULATOR_DEVICE_MODULE_MINOR_VERSION, SIMULATOR_DEVICE_MODULE_PATCH_VERSION),
          context,
          SIMULATOR_MODULE_ID)
{
}

ListPtr<IDeviceInfo> SimulatorDeviceModule::onGetAvailableDevices()
{
    const auto options = populateDefaultModuleOptions(this->context.getModuleOptions(SIMULATOR_MODULE_ID));
    return {SimulatorDeviceImpl::CreateDeviceInfo(options)};
}

DictPtr<IString, IDeviceType> SimulatorDeviceModule::onGetAvailableDeviceTypes()
{
    auto deviceType = SimulatorDeviceImpl::CreateType();
    return Dict<IString, IBaseObject>({{deviceType.getId(), deviceType}});
}

DevicePtr SimulatorDeviceModule::onCreateDevice(const StringPtr& connectionString,
                                                const ComponentPtr& parent,
                                                const PropertyObjectPtr& config)
{
    std::scoped_lock lock(sync);

    clearRemovedDevice();
    if (device.assigned())
        DAQ_THROW_EXCEPTION(AlreadyExistsException, "Simulator device is already created!");

    const auto options = populateDefaultModuleOptions(this->context.getModuleOptions(SIMULATOR_MODULE_ID));
    auto info = SimulatorDeviceImpl::CreateDeviceInfo(options);

    auto devicePtr = createWithImplementation<IDevice, SimulatorDeviceImpl>(config, context, parent, info);
    device = devicePtr;
    return devicePtr.detach();
}

DictPtr<IString, IBaseObject> SimulatorDeviceModule::populateDefaultModuleOptions(const DictPtr<IString, IBaseObject>& inputOptions)
{
    std::string hostname = boost::asio::ip::host_name();
    auto defaultOptions = Dict<IString, IBaseObject>({
        {"Name", "AI Signal Simulator"}, 
        {"Manufacturer", "openDAQ"}, 
        {"SerialNumber", hostname},
        {"MacAddress", getMacAddress()}, 
        {"SoftwareRevision", fmt::format("{}.{}.{}",
        SIMULATOR_DEVICE_MODULE_MAJOR_VERSION,
        SIMULATOR_DEVICE_MODULE_MINOR_VERSION,
        SIMULATOR_DEVICE_MODULE_PATCH_VERSION)}});

    for (const auto& [key, value] : inputOptions)
    {
        if (defaultOptions.hasKey(key))
        {
            defaultOptions.set(key, value);
        }
    }

    return defaultOptions;
}

void SimulatorDeviceModule::clearRemovedDevice()
{
    const bool isNull = !device.assigned() || !device.getRef().assigned();
    if (isNull || device.getRef().isRemoved())
        device = nullptr;
}

END_NAMESPACE_SIMULATOR_DEVICE_MODULE
