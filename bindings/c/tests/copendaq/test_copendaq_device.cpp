#include <copendaq.h>

#include <gtest/gtest.h>

#include <opendaq/device_type_factory.h>
#include <opendaq/opendaq.h>
#include <opendaq/streaming_type_factory.h>

class COpendaqDeviceTest : public testing::Test
{
    void SetUp() override
    {
        auto context = daq::NullContext();
        daqContext* ctx = (daqContext*) context.getObject();

        daqInstance_createInstance(&instance, ctx, nullptr);
        daqInstance_getRootDevice(instance, &dev);
    }

    void TearDown() override
    {
        daqBaseObject_releaseRef(instance);
        daqBaseObject_releaseRef(dev);
    }

protected:
    daqInstance* instance = nullptr;
    daqDevice* dev = nullptr;
};

TEST_F(COpendaqDeviceTest, AddressInfo)
{
    daqAddressInfoBuilder* builder = nullptr;
    daqAddressInfoBuilder_createAddressInfoBuilder(&builder);

    daqString* connectionString = nullptr;
    daqString_createString(&connectionString, "daqref://device0");
    daqAddressInfoBuilder_setConnectionString(builder, connectionString);
    daqAddressInfoBuilder_setReachabilityStatus(builder, daqAddressReachabilityStatusUnknown);
    daqString* type = nullptr;
    daqString_createString(&type, "Type");
    daqAddressInfoBuilder_setType(builder, type);
    daqBaseObject_releaseRef(type);
    daqString* address = nullptr;
    daqString_createString(&address, "Address");
    daqAddressInfoBuilder_setAddress(builder, connectionString);
    daqAddressInfoBuilder_setAddress(builder, address);
    daqBaseObject_releaseRef(address);
    daqBaseObject_releaseRef(connectionString);

    daqAddressInfo* addressInfo = nullptr;
    daqAddressInfoBuilder_build(builder, &addressInfo);
    ASSERT_NE(addressInfo, nullptr);

    daqString* connectionStringOut = nullptr;
    daqAddressInfo_getConnectionString(addressInfo, &connectionStringOut);
    ASSERT_NE(connectionStringOut, nullptr);

    daqBaseObject_releaseRef(connectionStringOut);
    daqBaseObject_releaseRef(addressInfo);
    daqBaseObject_releaseRef(builder);
}

TEST_F(COpendaqDeviceTest, DeviceInfo)
{
    daqDeviceInfo* deviceInfo = nullptr;
    daqDevice_getInfo(dev, &deviceInfo);
    ASSERT_NE(deviceInfo, nullptr);
    daqString* connectionString = nullptr;
    daqDeviceInfo_getConnectionString(deviceInfo, &connectionString);
    ASSERT_NE(connectionString, nullptr);
    daqConstCharPtr connectionStringStr = nullptr;
    daqString_getCharPtr(connectionString, &connectionStringStr);

    daqString* name = nullptr;
    daqDeviceInfo_getName(deviceInfo, &name);
    daqConstCharPtr nameStr = nullptr;
    daqString_getCharPtr(name, &nameStr);

    daqBaseObject_releaseRef(name);
    daqBaseObject_releaseRef(connectionString);
    daqBaseObject_releaseRef(deviceInfo);
}

TEST_F(COpendaqDeviceTest, IoFolderConfig)
{
    daqComponent* component = nullptr;
    daqBaseObject_borrowInterface(instance, DAQ_COMPONENT_INTF_ID, (daqBaseObject**) &component);
    ASSERT_NE(component, nullptr);

    daqFolderConfig* folderConfig = nullptr;
    daqString* localId = nullptr;
    daqString_createString(&localId, "IoFolder");
    daqContext* ctx = nullptr;
    daqComponent_getContext(component, &ctx);
    ASSERT_NE(ctx, nullptr);
    daqFolderConfig_createIoFolder(&folderConfig, ctx, nullptr, localId);
    ASSERT_NE(folderConfig, nullptr);

    daqBaseObject_releaseRef(localId);
    daqBaseObject_releaseRef(folderConfig);
    daqBaseObject_releaseRef(ctx);
}

TEST_F(COpendaqDeviceTest, DeviceType)
{
    const daq::StringPtr id = "id";
    const daq::StringPtr name = "name";
    const daq::StringPtr description = "description";
    const daq::StringPtr prefix = "prefix";
    const auto defaultConfig = daq::PropertyObject();
    defaultConfig.addProperty(daq::StringProperty("foo", "bar"));
    const auto methods = daq::Dict<daq::IString, daq::IAuthenticationMethod>(
        {{"none", daq::AuthenticationMethod("none", daq::Dict<daq::IString, daq::ICredentialField>(), "")}});

    daqDeviceType* deviceType = nullptr;
    daqDeviceType_createDeviceType(&deviceType,
                                   (daqString*) id.getObject(),
                                   (daqString*) name.getObject(),
                                   (daqString*) description.getObject(),
                                   (daqPropertyObject*) defaultConfig.getObject(),
                                   (daqString*) prefix.getObject(),
                                   (daqDict*) methods.getObject());
    ASSERT_NE(deviceType, nullptr);
    daqComponentType* componentType = nullptr;
    daqBaseObject_borrowInterface(deviceType, DAQ_COMPONENT_TYPE_INTF_ID, (daqBaseObject**) &componentType);
    ASSERT_NE(componentType, nullptr);

    daqString* str = nullptr;
    daqComponentType_getId(componentType, &str);
    ASSERT_EQ(daq::StringPtr::Adopt((daq::IString*) str), "id");
    daqComponentType_getName(componentType, &str);
    ASSERT_EQ(daq::StringPtr::Adopt((daq::IString*) str), "name");
    daqComponentType_getDescription(componentType, &str);
    ASSERT_EQ(daq::StringPtr::Adopt((daq::IString*) str), "description");
    daqDeviceType_getConnectionStringPrefix(deviceType, &str);
    ASSERT_EQ(daq::StringPtr::Adopt((daq::IString*) str), "prefix");

    daqModuleInfo* moduleInfo = nullptr;
    daqComponentType_getModuleInfo(componentType, &moduleInfo);
    ASSERT_EQ(moduleInfo, nullptr);

    daqPropertyObject* config = nullptr;
    daqComponentType_createDefaultConfig(componentType, &config);
    ASSERT_EQ(daq::PropertyObjectPtr::Adopt((daq::IPropertyObject*) config).getPropertyValue("foo"), "bar");

    daqAuthenticationConfig* authConfig = nullptr;
    daqComponentType_createAuthenticationConfig(componentType, &authConfig);
    ASSERT_TRUE(daq::AuthenticationConfigPtr::Adopt((daq::IAuthenticationConfig*) authConfig).getAuthenticationMethods().hasKey("none"));

    daqBaseObject_releaseRef(deviceType);
}

TEST_F(COpendaqDeviceTest, DeviceKind)
{
    static_assert(daqDeviceKindUnspecified == static_cast<int>(daq::DeviceKind::Unspecified));
    static_assert(daqDeviceKindAdapter == static_cast<int>(daq::DeviceKind::Adapter));
    static_assert(daqDeviceKindMirrored == static_cast<int>(daq::DeviceKind::Mirrored));
    static_assert(daqDeviceKindRoot == static_cast<int>(daq::DeviceKind::Root));

    daqComponentTypeBuilder* builder = nullptr;
    daqComponentTypeBuilder_createDeviceTypeBuilder(&builder);
    daqString* id = nullptr;
    daqString_createString(&id, "deviceType");
    daqComponentTypeBuilder_setId(builder, id);
    daqString* prefix = nullptr;
    daqString_createString(&prefix, "prefix");
    daqComponentTypeBuilder_setConnectionStringPrefix(builder, prefix);

    daqComponentTypeBuilder_setDeviceKind(builder, daqDeviceKindMirrored);
    daqDeviceKind kind = daqDeviceKindUnspecified;
    daqComponentTypeBuilder_getDeviceKind(builder, &kind);
    ASSERT_EQ(kind, daqDeviceKindMirrored);

    daqComponentType* componentType = nullptr;
    daqComponentTypeBuilder_build(builder, &componentType);
    ASSERT_NE(componentType, nullptr);

    daqDeviceType* deviceType = nullptr;
    daqBaseObject_borrowInterface(componentType, DAQ_DEVICE_TYPE_INTF_ID, (daqBaseObject**) &deviceType);
    ASSERT_NE(deviceType, nullptr);
    kind = daqDeviceKindUnspecified;
    daqDeviceType_getDeviceKind(deviceType, &kind);
    ASSERT_EQ(kind, daqDeviceKindMirrored);

    daqBaseObject_releaseRef(componentType);
    daqBaseObject_releaseRef(prefix);
    daqBaseObject_releaseRef(id);
    daqBaseObject_releaseRef(builder);
}

TEST_F(COpendaqDeviceTest, AddDeviceConfig)
{
    static_assert(daqAddressTypeIPv4 == static_cast<int>(daq::AddressType::IPv4));
    static_assert(daqAddressTypeIPv6 == static_cast<int>(daq::AddressType::IPv6));
    static_assert(daqStreamingModeDisabled == static_cast<int>(daq::StreamingMode::Disabled));
    static_assert(daqStreamingModeMinHops == static_cast<int>(daq::StreamingMode::MinHops));
    static_assert(daqStreamingModeMinConnections == static_cast<int>(daq::StreamingMode::MinConnections));

    const auto deviceType = daq::DeviceType("deviceType", "", "", "dev", daq::PropertyObject());
    const auto streamingType = daq::StreamingType("streamingType", "", "", "stream", daq::PropertyObject());
    const auto streamingTypes = daq::Dict<daq::IString, daq::IStreamingType>({{"streamingType", streamingType}});

    daqAddDeviceConfig* config = nullptr;
    daqAddDeviceConfig_createAddDeviceConfig(&config, (daqDeviceType*) deviceType.getObject(), (daqDict*) streamingTypes.getObject(), True);
    const auto configPtr = daq::AddDeviceConfigPtr::Adopt((daq::IAddDeviceConfig*) config);

    daqDeviceType* type = nullptr;
    daqAddDeviceConfig_getDeviceType(config, &type);
    ASSERT_EQ(daq::DeviceTypePtr::Adopt((daq::IDeviceType*) type), deviceType);

    daqPropertyObject* typeConfig = nullptr;
    daqAddDeviceConfig_getTypeConfig(config, &typeConfig);
    ASSERT_EQ(daq::PropertyObjectPtr::Adopt((daq::IPropertyObject*) typeConfig), configPtr.getTypeConfig());

    daqAddDeviceConfig_setPrimaryAddressType(config, daqAddressTypeIPv6);
    daqAddressType addressType = daqAddressTypeIPv4;
    daqAddDeviceConfig_getPrimaryAddressType(config, &addressType);
    ASSERT_EQ(addressType, daqAddressTypeIPv6);

    daqAddDeviceConfig_setScanOnAdd(config, False);
    daqBool scanOnAdd = True;
    daqAddDeviceConfig_getScanOnAdd(config, &scanOnAdd);
    ASSERT_FALSE(scanOnAdd);

    daqAddStreamingOptions* options = nullptr;
    daqAddDeviceConfig_getStreamingOptions(config, &options);
    const auto optionsPtr = daq::AddStreamingOptionsPtr::Adopt((daq::IAddStreamingOptions*) options);
    ASSERT_EQ(optionsPtr, configPtr.getStreamingOptions());

    daqDict* configs = nullptr;
    daqAddStreamingOptions_getStreamingConfigs(options, &configs);
    const auto configsPtr = daq::DictPtr<daq::IString, daq::IAddStreamingConfig>::Adopt((daq::IDict*) configs);
    ASSERT_TRUE(configsPtr.hasKey("streamingType"));

    daqString* typeId = nullptr;
    daqString_createString(&typeId, "streamingType");
    daqAddStreamingOptions_setStreamingType(options, typeId);
    daqBaseObject_releaseRef(typeId);
    daqAddStreamingConfig* selected = nullptr;
    daqAddStreamingOptions_getStreamingConfig(options, &selected, nullptr);
    ASSERT_EQ(daq::AddStreamingConfigPtr::Adopt((daq::IAddStreamingConfig*) selected),
              optionsPtr.getStreamingConfigs().get("streamingType"));

    daqAddStreamingOptions_setStreamingMode(options, daqStreamingModeMinConnections);
    daqStreamingMode mode = daqStreamingModeDisabled;
    daqAddStreamingOptions_getStreamingMode(options, &mode);
    ASSERT_EQ(mode, daqStreamingModeMinConnections);
}

TEST_F(COpendaqDeviceTest, AddStreamingConfig)
{
    const auto streamingType = daq::StreamingType("streamingType", "", "", "stream", daq::PropertyObject());

    daqAddStreamingConfig* config = nullptr;
    daqAddStreamingConfig_createAddStreamingConfig(&config, (daqStreamingType*) streamingType.getObject());
    const auto configPtr = daq::AddStreamingConfigPtr::Adopt((daq::IAddStreamingConfig*) config);

    daqStreamingType* type = nullptr;
    daqAddStreamingConfig_getStreamingType(config, &type);
    ASSERT_EQ(daq::StreamingTypePtr::Adopt((daq::IStreamingType*) type), streamingType);

    daqPropertyObject* typeConfig = nullptr;
    daqAddStreamingConfig_getTypeConfig(config, &typeConfig);
    ASSERT_EQ(daq::PropertyObjectPtr::Adopt((daq::IPropertyObject*) typeConfig), configPtr.getTypeConfig());
}

TEST_F(COpendaqDeviceTest, CreateAddDeviceConfig)
{
    const auto instance = daq::Instance();
    auto* rootDevice = (daqDevice*) instance.getRootDevice().getObject();

    const auto info = daq::DeviceInfo("daqref://device0");
    const daq::StringPtr prefix = "daqref";

    daqAddDeviceConfig* config = nullptr;
    ASSERT_EQ(daqDevice_createAddDeviceConfig(rootDevice, &config, (daqDeviceInfo*) info.getObject(), (daqString*) prefix.getObject()),
              DAQ_SUCCESS);
    const auto configPtr = daq::AddDeviceConfigPtr::Adopt((daq::IAddDeviceConfig*) config);
    ASSERT_EQ(configPtr.getDeviceType().getId(), "daqref");
}

TEST_F(COpendaqDeviceTest, CreateAddStreamingConfigOfRootDevice)
{
    const auto instance = daq::Instance();
    auto* rootDevice = (daqDevice*) instance.getRootDevice().getObject();

    const daq::StringPtr connectionString = "daq.nd://host";

    daqAddStreamingConfig* config = nullptr;
    ASSERT_EQ(daqDevice_createAddStreamingConfig(rootDevice, &config, (daqString*) connectionString.getObject()), DAQ_ERR_NOT_SUPPORTED);
    daqClearErrorInfo();
}

TEST_F(COpendaqDeviceTest, RootDeviceAddDeviceConfigIsNull)
{
    daqAddDeviceConfig* config = nullptr;
    ASSERT_EQ(daqDevice_getAddDeviceConfig(dev, &config), DAQ_SUCCESS);
    ASSERT_EQ(config, nullptr);
}

TEST_F(COpendaqDeviceTest, AvailableStreamingTypesOfRootDeviceIsEmpty)
{
    daqDict* types = nullptr;
    ASSERT_EQ(daqDevice_getAvailableStreamingTypes(dev, &types), DAQ_SUCCESS);
    ASSERT_NE(types, nullptr);
    const auto typesPtr = daq::DictPtr<daq::IString, daq::IStreamingType>::Adopt((daq::IDict*) types);
    ASSERT_EQ(typesPtr.getCount(), 0u);
}
