#include <testutils/testutils.h>
#include <opendaq/module_manager_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/logger_factory.h>
#include "mock/mock_module.h"

#include <chrono>
#include <thread>

#include <opendaq/component_type_builder_factory.h>
#include <opendaq/device_info_config_ptr.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/module_manager_utils_ptr.h>
#include <opendaq/device_impl.h>
#include <opendaq/address_info_factory.h>
#include <opendaq/device_info_internal_ptr.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <cstring>

#if defined(_WIN32)
#include <iphlpapi.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#endif

using namespace daq;

class ModuleManagerTest : public testing::Test
{
protected:
    static const std::string SearchDir;

    ModuleManagerTest()
        : logger(Logger())
        , context(NullContext(logger))
    {
    }

    void TearDown() override
    {
        using namespace std::chrono_literals;

        // Wait for Async logger to flush
        std::this_thread::sleep_for(100ms);
    }

    LoggerPtr logger;
    ContextPtr context;
};

const std::string ModuleManagerTest::SearchDir{MODULE_TEST_DIR};

TEST_F(ModuleManagerTest, Create)
{
    auto manager = ModuleManager("[[none]]");
    ASSERT_TRUE(manager.assigned());
}

// TODO: Fix memory leak
//
//TEST_F(ModuleManagerTest, DefaultToCurrentDirectory)
//{
//    ASSERT_NO_THROW(ModuleManager(""));
//}

TEST_F(ModuleManagerTest, CreateNullPath)
{
    ASSERT_THROW(ModuleManager(nullptr), InvalidParameterException);
}

TEST_F(ModuleManagerTest, CreateInvalidPath)
{
    auto mngr = ModuleManager("in\\/@|id");
    ASSERT_NO_THROW(Context(nullptr, Logger(), nullptr, mngr, nullptr));
}

TEST_F(ModuleManagerTest, CreateNonExistentPath)
{
    std::string workingDir = SearchDir + "/doesNotExist";
    auto mngr = ModuleManager(workingDir);
    ASSERT_NO_THROW(Context(nullptr, Logger(), nullptr, mngr, nullptr));
}

TEST_F(ModuleManagerTest, CreateNotAFolder)
{
    std::string workingDir = SearchDir + "/file";
    auto mngr = ModuleManager(workingDir);
    ASSERT_NO_THROW(Context(nullptr, Logger(), nullptr, mngr, nullptr));
}

TEST_F(ModuleManagerTest, AddModule)
{
    auto manager = ModuleManager(SearchDir);
    manager.loadModules(NullContext());
    auto mock = createWithImplementation<IModule, MockModuleImpl>();

    ASSERT_NO_THROW(manager.addModule(mock));

    bool added = false;
    for (const auto& module : manager.getModules())
    {
        if (module == mock)
        {
            ASSERT_EQ(module.getModuleInfo().getId(), "mock");
            added = true;
        }
    }

    ASSERT_TRUE(added);
}

TEST_F(ModuleManagerTest, AddModuleNullStringId)
{
    auto manager = ModuleManager(SearchDir);
    manager.loadModules(NullContext());
    auto mock = createWithImplementation<IModule, MockModuleImpl>(nullptr);

    ASSERT_NO_THROW(manager.addModule(mock));

    bool added = false;
    for (const auto& module : manager.getModules())
    {
        if (module == mock)
        {
            ASSERT_EQ(module.getModuleInfo().getId(), nullptr);
            added = true;
        }
    }

    ASSERT_TRUE(added);
}

TEST_F(ModuleManagerTest, AddModuleEmptyStringId)
{
    auto manager = ModuleManager(SearchDir);
    manager.loadModules(NullContext());
    auto mock = createWithImplementation<IModule, MockModuleImpl>("");

    ASSERT_NO_THROW(manager.addModule(mock));

    bool added = false;
    for (const auto& module : manager.getModules())
    {
        if (module == mock)
        {
            ASSERT_EQ(module.getModuleInfo().getId(), "");
            added = true;
        }
    }

    ASSERT_TRUE(added);
}

TEST_F(ModuleManagerTest, AddDuplicateModule)
{
    auto manager = ModuleManager(SearchDir);
    manager.loadModules(NullContext());
    auto mock = createWithImplementation<IModule, MockModuleImpl>();

    ASSERT_NO_THROW(manager.addModule(mock));
    ASSERT_THROW(manager.addModule(mock), DuplicateItemException);
}

TEST_F(ModuleManagerTest, EnumDriver)
{
    auto moduleManager = ModuleManager(SearchDir);
    moduleManager.loadModules(NullContext());

    ListPtr<IModule> moduleDrivers = moduleManager.getModules();
    auto count = moduleDrivers.getCount();

    ASSERT_EQ(count, 3u);

    std::cout << "Module driver count: " << count << std::endl;

#ifdef __GNUC__
    for (const ObjectPtr<IModule>& module : moduleDrivers)
    {
        ModuleInfoPtr info;
        module->getModuleInfo(&info);

        std::cout << info.getName() << std::endl;
    }

#else
    for (const ModulePtr& module : moduleDrivers)
    {
        std::cout << module.getModuleInfo().getName() << std::endl;
    }
#endif
}

TEST_F(ModuleManagerTest, RegisterDaqTypes)
{
    auto typeManager = context.getTypeManager();

    ASSERT_TRUE(typeManager.hasType("PtpSyncInterface"));
    ASSERT_TRUE(typeManager.hasType("SyncInterfaceBase"));
    ASSERT_TRUE(typeManager.hasType("InterfaceClockSync"));
}

// Carries a manufacturer and serial number, so that discovery results can be matched to it
class ScanTestDevice : public Device
{
public:
    ScanTestDevice(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId)
        : Device(ctx, parent, localId)
        , info(DeviceInfo("daqmock://dev"))
    {
        info.setManufacturer("openDAQ");
        info.setSerialNumber("scan_test");
    }

protected:
    DeviceInfoPtr onGetInfo() override
    {
        return info;
    }

private:
    DeviceInfoConfigPtr info;
};

class MockModuleInternal : public MockModuleImpl
{
public:
    MockModuleInternal();

    daq::ErrCode INTERFACE_FUNC getAvailableDevices(daq::IList** availableDevices) override;
    daq::ErrCode INTERFACE_FUNC getAvailableDeviceTypes(daq::IDict** deviceTypes) override;
    daq::ErrCode INTERFACE_FUNC createDevice(daq::IDevice** device, daq::IString* connectionString, daq::IComponent* parent, daq::IPropertyObject* config) override;

    DeviceInfoConfigPtr info;
    DeviceTypePtr type;
    int scanCount = 0;
};

MockModuleInternal::MockModuleInternal()
{
    info = DeviceInfo("daqmock://dev");
    type = DeviceTypeBuilder().setConnectionStringPrefix("daqmock").setId("daqmock").build();
}

daq::ErrCode MockModuleInternal::getAvailableDevices(daq::IList** availableDevices)
{
    scanCount++;
    *availableDevices = List<IDeviceInfo>(info).detach();
    return OPENDAQ_SUCCESS;
}

daq::ErrCode MockModuleInternal::getAvailableDeviceTypes(daq::IDict** deviceTypes)
{
    *deviceTypes = Dict<IString, IDeviceType>({{type.getId(), type}}).detach();
    return OPENDAQ_SUCCESS;
}

daq::ErrCode MockModuleInternal::createDevice(daq::IDevice** device,
                                              daq::IString* connectionString,
                                              daq::IComponent* /*parent*/,
                                              daq::IPropertyObject* /*config*/)
{
    OPENDAQ_PARAM_NOT_NULL(connectionString);

    StringPtr connectionStringPtr = StringPtr::Borrow(connectionString);
    if (connectionStringPtr == "daqmock://invalid_arg")
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_ARGUMENT);
    else if (connectionStringPtr == "daqmock://not_found")
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
    else if (connectionStringPtr == "daqmock://general_error")
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, "abc123");

    // A context of its own keeps the device out of the module manager's ownership cycle
    *device = createWithImplementation<IDevice, ScanTestDevice>(NullContext(), nullptr, String("dev")).detach();
    return OPENDAQ_SUCCESS;
}

TEST_F(ModuleManagerTest, TestRescanTimer1)
{
    auto manager = ModuleManager("[[none]]");

    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 100000}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);
    
    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 1);
    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 1);
}

TEST_F(ModuleManagerTest, TestRescanTimer2)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 10}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);
    
    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 1);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 2);
}

TEST_F(ModuleManagerTest, ScanOnAddDisabled)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"ScanOnAdd", False}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 0);

    // a discovery lookup does not scan either
    ASSERT_FALSE(utils.getDiscoveryInfo("openDAQ", "missing").assigned());
    ASSERT_EQ(impl->scanCount, 0);

    // a smart connection string cannot be resolved without a scan
    ASSERT_THROW(utils.createDevice("daq://openDAQ_missing", nullptr), NotFoundException);
    ASSERT_EQ(impl->scanCount, 0);
}

TEST_F(ModuleManagerTest, ScanOnAddConfigProperty)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"ScanOnAdd", False}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    // the default config follows the option; the property overrides it per call
    auto config = utils.createDefaultAddDeviceConfig();
    PropertyObjectPtr general = config.getPropertyValue("General");
    ASSERT_EQ(static_cast<bool>(general.getPropertyValue("ScanOnAdd")), false);

    utils.createDevice("daqmock", nullptr, config);
    ASSERT_EQ(impl->scanCount, 0);

    general.setPropertyValue("ScanOnAdd", True);
    utils.createDevice("daqmock", nullptr, config);
    ASSERT_EQ(impl->scanCount, 1);
}

TEST_F(ModuleManagerTest, ScanOnAddDefaultEnabled)
{
    auto manager = ModuleManager("[[none]]");
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    auto config = utils.createDefaultAddDeviceConfig();
    PropertyObjectPtr general = config.getPropertyValue("General");
    ASSERT_EQ(static_cast<bool>(general.getPropertyValue("ScanOnAdd")), true);

    general.setPropertyValue("ScanOnAdd", False);
    utils.createDevice("daqmock", nullptr, config);
    ASSERT_EQ(impl->scanCount, 0);

    utils.createDevice("daqmock", nullptr);
    ASSERT_EQ(impl->scanCount, 1);
}

TEST_F(ModuleManagerTest, ParallelDeviceCreationSuccess)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 100000}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    auto connectionArgs = Dict<IString, IPropertyObject>({{"daqmock://1", nullptr}, {"daqmock://2", nullptr}});
    DictPtr<IString, IDevice> devices;
    ASSERT_NO_THROW(devices = utils.createDevices(connectionArgs, nullptr));
    ASSERT_EQ(devices.getCount(), 2u);
    ASSERT_EQ(impl->scanCount, 1);

    devices = Dict<IString, IDevice>();
    ASSERT_EQ(utils->createDevices(&devices, connectionArgs, nullptr), OPENDAQ_SUCCESS);

    auto errCodes = Dict<IString, IInteger>();
    auto errorInfos = Dict<IString, IErrorInfo>();
    ASSERT_NO_THROW(devices = utils.createDevices(connectionArgs, nullptr, errCodes, errorInfos));
    ASSERT_EQ(devices.getCount(), 2u);
    ASSERT_EQ(errCodes.getCount(), 2u);
    ASSERT_EQ(errorInfos.getCount(), 2u);

    ASSERT_EQ(errCodes.get("daqmock://1"), OPENDAQ_SUCCESS);
    ASSERT_EQ(errCodes.get("daqmock://2"), OPENDAQ_SUCCESS);
    ASSERT_FALSE(errorInfos.get("daqmock://1").assigned());
    ASSERT_FALSE(errorInfos.get("daqmock://2").assigned());
}

TEST_F(ModuleManagerTest, ParallelDeviceCreationPartialSuccess)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 100000}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto impl = reinterpret_cast<MockModuleInternal*>(module.getObject());
    auto utils = manager.asPtr<IModuleManagerUtils>();

    auto connectionArgs = Dict<IString, IPropertyObject>({{"daqmock://1", nullptr},
                                                          {"daqmock://invalid_arg", nullptr},
                                                          {"daqmock://general_error", nullptr}});
    DictPtr<IString, IDevice> devices;
    ASSERT_NO_THROW(devices = utils.createDevices(connectionArgs, nullptr));
    ASSERT_EQ(devices.getCount(), 3u);
    ASSERT_EQ(impl->scanCount, 1);

    devices = Dict<IString, IDevice>();
    ASSERT_EQ(utils->createDevices(&devices, connectionArgs, nullptr), OPENDAQ_PARTIAL_SUCCESS);

    auto errCodes = Dict<IString, IInteger>();
    auto errorInfos = Dict<IString, IErrorInfo>();
    ASSERT_NO_THROW(devices = utils.createDevices(connectionArgs, nullptr, errCodes, errorInfos));
    ASSERT_EQ(devices.getCount(), 3u);
    ASSERT_EQ(errCodes.getCount(), 3u);
    ASSERT_EQ(errorInfos.getCount(), 3u);

    ASSERT_EQ(errCodes.get("daqmock://1"), OPENDAQ_SUCCESS);
    ASSERT_EQ(errCodes.get("daqmock://invalid_arg"), OPENDAQ_ERR_INVALID_ARGUMENT);
    ASSERT_EQ(errCodes.get("daqmock://general_error"), OPENDAQ_ERR_GENERALERROR);
    ASSERT_FALSE(errorInfos.get("daqmock://1").assigned());
    ASSERT_TRUE(errorInfos.get("daqmock://invalid_arg").assigned());
    ASSERT_TRUE(errorInfos.get("daqmock://general_error").assigned());
}

TEST_F(ModuleManagerTest, ParallelDeviceCreationFailure)
{
    auto manager = ModuleManager("[[none]]");
    auto options = Dict<IString, IBaseObject>({{"ModuleManager", Dict<IString, IBaseObject>({{"AddDeviceRescanTimer", 100000}})}});
    const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr, options);

    auto module = createWithImplementation<IModule, MockModuleInternal>();
    manager.addModule(module);
    auto utils = manager.asPtr<IModuleManagerUtils>();

    ASSERT_THROW_MSG(utils.createDevices(Dict<IString, IPropertyObject>(), nullptr), InvalidParameterException, "None connection arguments provided");

    auto connectionArgs = Dict<IString, IPropertyObject>({{"daqmock://general_error", nullptr}});
    DictPtr<IString, IDevice> devices;
    auto errCodes = Dict<IString, IInteger>();
    auto errorInfos = Dict<IString, IErrorInfo>();

    ASSERT_ERROR_CODE_EQ(utils->createDevices(&devices, connectionArgs, nullptr, errCodes, errorInfos), OPENDAQ_ERR_GENERALERROR);
    ASSERT_EQ(devices.getCount(), 1u);
    ASSERT_EQ(errCodes.getCount(), 1u);
    ASSERT_EQ(errorInfos.getCount(), 1u);
    ASSERT_TRUE(devices.hasKey("daqmock://general_error"));
    ASSERT_TRUE(errCodes.hasKey("daqmock://general_error"));
    ASSERT_TRUE(errorInfos.hasKey("daqmock://general_error"));

    ObjectPtr<IErrorInfo> errorInfo = errorInfos.get("daqmock://general_error");
    ASSERT_EQ(errCodes.get("daqmock://general_error"), OPENDAQ_ERR_GENERALERROR);
    ASSERT_TRUE(errorInfo.assigned());
    // TODO search for substring within the backtrace information
//    StringPtr message;
//    errorInfo->getMessage(&message);
//    ASSERT_TRUE(message.toStdString().find("abc123") != std::string::npos);
}

// Reports one device whose capability lists the given addresses, in order, as the discovery modules do.
class PingTestModule : public MockModuleImpl
{
public:
    explicit PingTestModule(std::vector<std::string> addresses)
        : addresses(std::move(addresses))
    {
    }

    daq::ErrCode INTERFACE_FUNC getAvailableDevices(daq::IList** availableDevices) override
    {
        auto cap = ServerCapability("PingTest", "PingTest", ProtocolType::Configuration);
        cap.setConnectionType("TCP/IP");
        for (const auto& address : addresses)
            cap.addAddressInfo(AddressInfoBuilder()
                                   .setAddress(address)
                                   .setType(address.front() == '[' ? "IPv6" : "IPv4")
                                   .setConnectionString("daqping://" + address)
                                   .build());

        auto info = DeviceInfo("daqping://dev");
        info.setManufacturer("openDAQ");
        info.setSerialNumber("ping_test");
        info.asPtr<IDeviceInfoInternal>().addServerCapability(cap);
        *availableDevices = List<IDeviceInfo>(info).detach();
        return OPENDAQ_SUCCESS;
    }

private:
    std::vector<std::string> addresses;
};

class ModuleManagerPingTest : public ModuleManagerTest
{
protected:
    ServerCapabilityPtr scan(const std::vector<std::string>& addresses)
    {
        auto manager = ModuleManager("[[none]]");
        const auto context = Context(nullptr, Logger(), nullptr, manager, nullptr);
        manager.addModule(createWithImplementation<IModule, PingTestModule>(addresses));
        return manager.asPtr<IModuleManagerUtils>().getAvailableDevices()[0].getServerCapabilities()[0];
    }

    static AddressReachabilityStatus status(const ServerCapabilityPtr& cap, const std::string& address)
    {
        for (const auto& info : cap.getAddressInfo())
            if (info.getAddress() == address)
                return info.getReachabilityStatus();
        throw std::runtime_error("No address info for " + address);
    }

    static bool hasIpv6Loopback()
    {
        boost::asio::io_context ioContext;
        boost::asio::ip::udp::socket socket(ioContext);
        boost::system::error_code ec;
        socket.open(boost::asio::ip::udp::v6(), ec);
        if (!ec)
            socket.bind({boost::asio::ip::address_v6::loopback(), 0}, ec);
        return !ec;
    }

    // Returns the host's own non-loopback addresses as discovery reports them: IPv6 in brackets, link-local with its scope.
    // Point-to-point interfaces (macOS utun tunnels) are left out: a ping to their own address goes to the tunnel peer.
    static std::vector<std::string> interfaceAddresses()
    {
        using namespace boost::asio::ip;
        std::vector<std::string> result;
        const auto add = [&result](const sockaddr* raw, unsigned index)
        {
            if (raw->sa_family == AF_INET)
            {
                address_v4::bytes_type bytes;
                std::memcpy(bytes.data(), &reinterpret_cast<const sockaddr_in*>(raw)->sin_addr, bytes.size());
                result.push_back(address_v4(bytes).to_string());
            }
            else if (raw->sa_family == AF_INET6)
            {
                address_v6::bytes_type bytes;
                std::memcpy(bytes.data(), &reinterpret_cast<const sockaddr_in6*>(raw)->sin6_addr, bytes.size());
                const bool linkLocal = address_v6(bytes).is_link_local();
                if (linkLocal)
                    bytes[2] = bytes[3] = 0;  // macOS keeps the interface index there
                result.push_back("[" + address_v6(bytes, linkLocal ? index : 0).to_string() + "]");
            }
        };

#if defined(_WIN32)
        ULONG size = 0;
        GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &size);
        std::vector<char> buffer(size);
        auto* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        if (GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, adapters, &size) != NO_ERROR)
            return result;
        for (auto* adapter = adapters; adapter; adapter = adapter->Next)
            if (adapter->OperStatus == IfOperStatusUp && adapter->IfType != IF_TYPE_SOFTWARE_LOOPBACK)
                for (auto* unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
                    if (unicast->DadState == IpDadStatePreferred)
                        add(unicast->Address.lpSockaddr, adapter->Ipv6IfIndex);
#else
        ifaddrs* interfaces = nullptr;
        if (getifaddrs(&interfaces) != 0)
            return result;
        for (auto* i = interfaces; i; i = i->ifa_next)
            if (i->ifa_addr && (i->ifa_flags & (IFF_UP | IFF_RUNNING | IFF_LOOPBACK | IFF_POINTOPOINT)) == (IFF_UP | IFF_RUNNING))
                add(i->ifa_addr, if_nametoindex(i->ifa_name));
        freeifaddrs(interfaces);
#endif
        return result;
    }

    // Returns why the test cannot run: no IPv6 loopback, or no ICMP permission (Linux without root outside net.ipv4.ping_group_range).
    std::string pingUnavailable(bool ipv6)
    {
        if (ipv6 && !hasIpv6Loopback())
            return "The host has no IPv6 loopback";
        const std::string loopback = ipv6 ? "[::1]" : "127.0.0.1";
        if (status(scan({loopback}), loopback) == AddressReachabilityStatus::Unknown)
            return "This process may not send ICMP";
        return {};
    }
};

TEST_F(ModuleManagerPingTest, Ipv4LoopbackReachable)
{
    if (const auto reason = pingUnavailable(false); !reason.empty())
        GTEST_SKIP() << reason;
    ASSERT_EQ(status(scan({"127.0.0.1"}), "127.0.0.1"), AddressReachabilityStatus::Reachable);
}

TEST_F(ModuleManagerPingTest, Ipv6LoopbackReachable)
{
    if (const auto reason = pingUnavailable(true); !reason.empty())
        GTEST_SKIP() << reason;
    ASSERT_EQ(status(scan({"[::1]"}), "[::1]"), AddressReachabilityStatus::Reachable);
}

TEST_F(ModuleManagerPingTest, InterfaceAddressesReachable)
{
    if (const auto reason = pingUnavailable(false); !reason.empty())
        GTEST_SKIP() << reason;
    const auto addresses = interfaceAddresses();
    if (addresses.empty())
        GTEST_SKIP() << "The host has no network interface besides loopback";
    const auto cap = scan(addresses);
    for (const auto& address : addresses)
        EXPECT_EQ(status(cap, address), AddressReachabilityStatus::Reachable) << address;
}

TEST_F(ModuleManagerPingTest, DocumentationAddressesUnreachable)
{
    if (const auto reason = pingUnavailable(false); !reason.empty())
        GTEST_SKIP() << reason;
    const auto cap = scan({"192.0.2.1", "[2001:db8::1]"});
    ASSERT_EQ(status(cap, "192.0.2.1"), AddressReachabilityStatus::Unreachable);
    ASSERT_EQ(status(cap, "[2001:db8::1]"), AddressReachabilityStatus::Unreachable);
}

TEST_F(ModuleManagerPingTest, Ipv6ChosenWhenIpv4Unreachable)
{
    if (const auto reason = pingUnavailable(true); !reason.empty())
        GTEST_SKIP() << reason;
    const auto cap = scan({"192.0.2.1", "[::1]"});
    ASSERT_EQ(status(cap, "192.0.2.1"), AddressReachabilityStatus::Unreachable);
    ASSERT_EQ(cap.getConnectionString(), "daqping://[::1]");
}

TEST_F(ModuleManagerPingTest, Ipv4KeptWhenBothReachable)
{
    if (const auto reason = pingUnavailable(true); !reason.empty())
        GTEST_SKIP() << reason;
    const auto cap = scan({"127.0.0.1", "[::1]"});
    ASSERT_EQ(status(cap, "127.0.0.1"), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(status(cap, "[::1]"), AddressReachabilityStatus::Reachable);
    ASSERT_EQ(cap.getConnectionString(), "daqping://127.0.0.1");
}

TEST_F(ModuleManagerPingTest, UnparsableAddressUnknown)
{
    const auto cap = scan({"not-an-address"});
    ASSERT_EQ(status(cap, "not-an-address"), AddressReachabilityStatus::Unknown);
    ASSERT_EQ(cap.getConnectionString(), "daqping://not-an-address");
}
