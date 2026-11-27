#include <copendaq.h>

#include <gtest/gtest.h>

#include <opendaq/opendaq.h>

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
    static_assert(daqStreamingModeMinHops == static_cast<int>(daq::StreamingMode::MinHops));
    static_assert(daqStreamingModeMinConnections == static_cast<int>(daq::StreamingMode::MinConnections));

    const auto instance = daq::Instance();
    const auto info = daq::DeviceInfo("daq.nd://127.0.0.1");

    daqAddDeviceConfig* config = nullptr;
    ASSERT_EQ(daqDevice_createAddDeviceConfig((daqDevice*) instance.getRootDevice().getObject(), &config, (daqDeviceInfo*) info.getObject(), nullptr),
              DAQ_SUCCESS);
    const auto configPtr = daq::AddDeviceConfigPtr::Adopt((daq::IAddDeviceConfig*) config);

    daqDeviceType* type = nullptr;
    daqAddDeviceConfig_getDeviceType(config, &type);
    ASSERT_EQ(daq::DeviceTypePtr::Adopt((daq::IDeviceType*) type), configPtr.getDeviceType());

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
    daqAddStreamingOptions_getAvailableStreamingConfigs(options, &configs);
    const auto configsPtr = daq::DictPtr<daq::IString, daq::IAddStreamingConfig>::Adopt((daq::IDict*) configs);
    ASSERT_TRUE(configsPtr.hasKey("OpenDAQNativeStreaming"));

    daqAddStreamingOptions_setStreamingType(options, nullptr);
    daqString* typeId = nullptr;
    daqAddStreamingOptions_getStreamingType(options, &typeId);
    ASSERT_EQ(typeId, nullptr);

    daqString_createString(&typeId, "OpenDAQNativeStreaming");
    daqAddStreamingOptions_setStreamingType(options, typeId);
    daqBaseObject_releaseRef(typeId);
    daqAddStreamingOptions_getStreamingType(options, &typeId);
    ASSERT_EQ(daq::StringPtr::Adopt((daq::IString*) typeId), "OpenDAQNativeStreaming");
    daqAddStreamingConfig* selected = nullptr;
    daqAddStreamingOptions_getStreamingConfig(options, &selected);
    const auto selectedPtr = daq::AddStreamingConfigPtr::Adopt((daq::IAddStreamingConfig*) selected);
    ASSERT_EQ(selectedPtr, optionsPtr.getAvailableStreamingConfigs().get("OpenDAQNativeStreaming"));

    daqStreamingType* streamingType = nullptr;
    daqAddStreamingConfig_getStreamingType(selected, &streamingType);
    ASSERT_EQ(daq::StreamingTypePtr::Adopt((daq::IStreamingType*) streamingType), selectedPtr.getStreamingType());

    daqPropertyObject* streamingTypeConfig = nullptr;
    daqAddStreamingConfig_getTypeConfig(selected, &streamingTypeConfig);
    ASSERT_EQ(daq::PropertyObjectPtr::Adopt((daq::IPropertyObject*) streamingTypeConfig), selectedPtr.getTypeConfig());

    daqAddStreamingOptions_setStreamingMode(options, daqStreamingModeMinConnections);
    daqStreamingMode mode = daqStreamingModeMinHops;
    daqAddStreamingOptions_getStreamingMode(options, &mode);
    ASSERT_EQ(mode, daqStreamingModeMinConnections);
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

using TransportOptionsPtr = daq::DictPtr<daq::IString, daq::ITransportOption>;

TEST_F(COpendaqDeviceTest, TransportOption)
{
    static_assert(daqSecurityModeNone == static_cast<int>(daq::SecurityMode::None));
    static_assert(daqSecurityModeSigned == static_cast<int>(daq::SecurityMode::Signed));
    static_assert(daqSecurityModeEncrypted == static_cast<int>(daq::SecurityMode::Encrypted));

    const auto builder = daq::TransportOptionBuilder("tls").setPort(7422).setSecurityMode(daq::SecurityMode::Encrypted).setCertificateRequired(True);
    daqTransportOption* option = nullptr;
    ASSERT_EQ(daqTransportOption_createTransportOptionFromBuilder(&option, (daqTransportOptionBuilder*) builder.getObject()), DAQ_SUCCESS);
    const auto optionPtr = daq::TransportOptionPtr::Adopt((daq::ITransportOption*) option);

    daqString* optionId = nullptr;
    daqTransportOption_getId(option, &optionId);
    ASSERT_EQ(daq::StringPtr::Adopt((daq::IString*) optionId), "tls");

    daqInt port = 0;
    daqTransportOption_getPort(option, &port);
    ASSERT_EQ(port, 7422);

    daqSecurityMode mode = daqSecurityModeNone;
    daqTransportOption_getSecurityMode(option, &mode);
    ASSERT_EQ(mode, daqSecurityModeEncrypted);

    daqBool required = False;
    daqTransportOption_getCertificateRequired(option, &required);
    ASSERT_TRUE(required);
}

TEST_F(COpendaqDeviceTest, TransportOptionBuilder)
{
    const daq::StringPtr id = "tls";
    daqTransportOptionBuilder* builder = nullptr;
    ASSERT_EQ(daqTransportOptionBuilder_createTransportOptionBuilder(&builder, (daqString*) id.getObject()), DAQ_SUCCESS);
    const auto builderPtr = daq::TransportOptionBuilderPtr::Adopt((daq::ITransportOptionBuilder*) builder);

    daqString* builderId = nullptr;
    daqTransportOptionBuilder_getId(builder, &builderId);
    ASSERT_EQ(daq::StringPtr::Adopt((daq::IString*) builderId), "tls");

    ASSERT_EQ(daqTransportOptionBuilder_setPort(builder, 7422), DAQ_SUCCESS);
    ASSERT_EQ(daqTransportOptionBuilder_setSecurityMode(builder, daqSecurityModeEncrypted), DAQ_SUCCESS);
    ASSERT_EQ(daqTransportOptionBuilder_setCertificateRequired(builder, True), DAQ_SUCCESS);

    daqInt builderPort = 0;
    daqTransportOptionBuilder_getPort(builder, &builderPort);
    ASSERT_EQ(builderPort, 7422);

    daqTransportOption* built = nullptr;
    ASSERT_EQ(daqTransportOptionBuilder_build(builder, &built), DAQ_SUCCESS);
    const auto builtPtr = daq::TransportOptionPtr::Adopt((daq::ITransportOption*) built);
    ASSERT_EQ(builtPtr.getId(), "tls");
    ASSERT_EQ(builtPtr.getPort(), 7422);
    ASSERT_EQ(builtPtr.getSecurityMode(), daq::SecurityMode::Encrypted);
    ASSERT_TRUE(builtPtr.getCertificateRequired());

    daqTransportOptionBuilder* copy = nullptr;
    ASSERT_EQ(daqTransportOptionBuilder_createTransportOptionBuilderFromExisting(&copy, built), DAQ_SUCCESS);
    const auto copyPtr = daq::TransportOptionBuilderPtr::Adopt((daq::ITransportOptionBuilder*) copy);
    ASSERT_EQ(copyPtr.getId(), "tls");
    ASSERT_EQ(copyPtr.getSecurityMode(), daq::SecurityMode::Encrypted);
}

TEST_F(COpendaqDeviceTest, ComponentTypeTransportOptions)
{
    const auto options = daq::Dict<daq::IString, daq::ITransportOption>({{"tls", daq::TransportOptionBuilder("tls").setPort(7422).build()}});
    const daq::StringPtr id = "dev";

    daqComponentTypeBuilder* builder = nullptr;
    daqComponentTypeBuilder_createComponentTypeBuilder(&builder);
    const auto builderPtr = daq::ComponentTypeBuilderPtr::Adopt((daq::IComponentTypeBuilder*) builder);
    daqComponentTypeBuilder_setTypeSort(builder, daqComponentTypeSortDevice);
    daqComponentTypeBuilder_setId(builder, (daqString*) id.getObject());
    ASSERT_EQ(daqComponentTypeBuilder_setAvailableTransportOptions(builder, (daqDict*) options.getObject()), DAQ_SUCCESS);

    daqDict* builderOptions = nullptr;
    daqComponentTypeBuilder_getAvailableTransportOptions(builder, &builderOptions);
    ASSERT_EQ(TransportOptionsPtr::Adopt((daq::IDict*) builderOptions).getKeyList(),
              daq::List<daq::IString>("tls"));

    daqComponentType* type = nullptr;
    daqComponentTypeBuilder_build(builder, &type);
    const auto typePtr = daq::ComponentTypePtr::Adopt((daq::IComponentType*) type);

    daqDict* typeOptions = nullptr;
    daqComponentType_getAvailableTransportOptions(type, &typeOptions);
    const TransportOptionsPtr typeOptionsPtr = TransportOptionsPtr::Adopt((daq::IDict*) typeOptions);
    ASSERT_EQ(typeOptionsPtr.getCount(), 1u);
    ASSERT_EQ(daq::TransportOptionPtr(typeOptionsPtr.get("tls")).getPort(), 7422);
}

TEST_F(COpendaqDeviceTest, ServerCapabilityTransportOptions)
{
    const daq::StringPtr protocolId = "protocol";
    daqServerCapabilityConfig* capability = nullptr;
    daqServerCapabilityConfig_createServerCapability(
        &capability, (daqString*) protocolId.getObject(), (daqString*) protocolId.getObject(), daqProtocolTypeConfiguration);
    const auto capabilityPtr = daq::ServerCapabilityConfigPtr::Adopt((daq::IServerCapabilityConfig*) capability);

    const auto options = daq::Dict<daq::IString, daq::ITransportOption>({{"tls", daq::TransportOptionBuilder("tls").setPort(7422).build()}});
    ASSERT_EQ(daqServerCapabilityConfig_setAvailableTransportOptions(capability, (daqDict*) options.getObject()), DAQ_SUCCESS);
    const auto plain = daq::TransportOptionBuilder("plain").setPort(7420).build();
    ASSERT_EQ(daqServerCapabilityConfig_addAvailableTransportOption(capability, (daqTransportOption*) plain.getObject()), DAQ_SUCCESS);

    daqServerCapability* readable = nullptr;
    daqBaseObject_borrowInterface(capability, DAQ_SERVER_CAPABILITY_INTF_ID, (daqBaseObject**) &readable);
    daqDict* capabilityOptions = nullptr;
    daqServerCapability_getAvailableTransportOptions(readable, &capabilityOptions);
    ASSERT_EQ(TransportOptionsPtr::Adopt((daq::IDict*) capabilityOptions).getKeyList(),
              daq::List<daq::IString>("tls", "plain"));
}

TEST_F(COpendaqDeviceTest, AddConfigTransportOption)
{
    const auto options = daq::Dict<daq::IString, daq::ITransportOption>(
        {{"tls", daq::TransportOptionBuilder("tls").setPort(7422).build()}, {"plain", daq::TransportOptionBuilder("plain").setPort(7420).build()}});
    const auto deviceType = daq::DeviceTypeBuilder()
                                .setId("dev")
                                .setConnectionStringPrefix("dev")
                                .setDeviceKind(daq::DeviceKind::Mirrored)
                                .setAvailableTransportOptions(options)
                                .build();
    const auto streamingType =
        daq::StreamingTypeBuilder().setId("str").setConnectionStringPrefix("str").setAvailableTransportOptions(options).build();
    const auto configPtr =
        daq::AddDeviceConfig(deviceType, daq::Dict<daq::IString, daq::IStreamingType>({{"str", streamingType}}), nullptr, True);
    auto* config = (daqAddDeviceConfig*) configPtr.getObject();

    daqDict* configOptions = nullptr;
    daqAddDeviceConfig_getAvailableTransportOptions(config, &configOptions);
    ASSERT_EQ(TransportOptionsPtr::Adopt((daq::IDict*) configOptions).getCount(), 2u);

    const daq::StringPtr plain = "plain";
    ASSERT_EQ(daqAddDeviceConfig_setTransportOption(config, (daqString*) plain.getObject()), DAQ_SUCCESS);
    daqTransportOption* selected = nullptr;
    daqAddDeviceConfig_getSelectedTransportOption(config, &selected);
    ASSERT_EQ(daq::TransportOptionPtr::Adopt((daq::ITransportOption*) selected).getId(), "plain");

    daqInt port = 0;
    daqAddDeviceConfig_getPort(config, &port);
    ASSERT_EQ(port, 7420);
    ASSERT_EQ(daqAddDeviceConfig_setPort(config, 7500), DAQ_SUCCESS);
    daqAddDeviceConfig_getPort(config, &port);
    ASSERT_EQ(port, 7500);

    auto* streamingConfig = (daqAddStreamingConfig*) configPtr.getStreamingOptions().getAvailableStreamingConfigs().get("str").getObject();
    daqAddStreamingConfig_getAvailableTransportOptions(streamingConfig, &configOptions);
    ASSERT_EQ(TransportOptionsPtr::Adopt((daq::IDict*) configOptions).getCount(), 2u);
    ASSERT_EQ(daqAddStreamingConfig_setTransportOption(streamingConfig, (daqString*) plain.getObject()), DAQ_SUCCESS);
    daqAddStreamingConfig_getSelectedTransportOption(streamingConfig, &selected);
    ASSERT_EQ(daq::TransportOptionPtr::Adopt((daq::ITransportOption*) selected).getId(), "plain");
    ASSERT_EQ(daqAddStreamingConfig_setPort(streamingConfig, 7600), DAQ_SUCCESS);
    daqAddStreamingConfig_getPort(streamingConfig, &port);
    ASSERT_EQ(port, 7600);
}
