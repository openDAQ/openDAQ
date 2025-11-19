#include <opendaq/device_impl.h>
#include <opendaq/device_ptr.h>
#include <opendaq/device_info_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/folder_impl.h>
#include <opendaq/channel_impl.h>
#include <opendaq/signal_impl.h>
#include <opendaq/io_folder_impl.h>
#include <opendaq/io_folder_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/search_filter_factory.h>

using TreeTraversalTest = testing::Test;
using namespace daq;
using namespace daq::search;

class TestSignal : public SignalImpl
{
public:
    TestSignal(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& id, Bool visible) 
        : SignalImpl(context, nullptr, parent, id)
    {
        this->visible = visible;

        objPtr.addProperty(StringProperty("SignalProp", "DefaultValue"));
        objPtr.addProperty(StringProperty("CommonProp", "DefaultValue"));
    }
};

class TestFunctionBlock: public FunctionBlockImpl<>
{
public:
    TestFunctionBlock(const ContextPtr& context, const ComponentPtr& parent,  const StringPtr& id, Bool visible) 
        : FunctionBlockImpl<>(nullptr, context, parent, id)
    {
        this->visible = visible;
        
        this->functionBlocks.addProperty(StringProperty("CommonProp", "DefaultValue"));

        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigVis", true));
        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigInvis", false));
        this->signals.addProperty(StringProperty("CommonProp", "DefaultValue"));

        auto inputPort = InputPort(context, this->inputPorts, "ip");
        inputPort.addProperty(StringProperty("InputPortProp", "DefaultValue"));
        inputPort.addProperty(StringProperty("CommonProp", "DefaultValue"));

        this->inputPorts.addItem(inputPort);
        this->inputPorts.addProperty(StringProperty("CommonProp", "DefaultValue"));

        objPtr.addProperty(StringProperty("FunctionBlockProp", "DefaultValue"));
        objPtr.addProperty(StringProperty("CommonProp", "DefaultValue"));
    }
};

class TestChannel : public ChannelImpl<>
{
public:
    TestChannel(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& id, Bool visible, Bool isRoot = true) 
        : ChannelImpl<>(nullptr, context, parent, id)
    {
        this->visible = visible;

        this->inputPorts.addProperty(StringProperty("CommonProp", "DefaultValue"));

        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigVis", true));
        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigInvis", false));
        this->signals.addProperty(StringProperty("CommonProp", "DefaultValue"));

        this->functionBlocks.addItem(createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, this->functionBlocks, "fbVis", true));
        this->functionBlocks.addItem(createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, this->functionBlocks, "fbInvis", false));
        this->functionBlocks.addProperty(StringProperty("CommonProp", "DefaultValue"));

        objPtr.addProperty(StringProperty("ChannelProp", "DefaultValue"));
        objPtr.addProperty(StringProperty("CommonProp", "DefaultValue"));
    }
};

class TestIOFolder : public IoFolderImpl<>
{
public:
    TestIOFolder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& id, Bool visible, Bool isRoot = true) 
        : IoFolderImpl(context, parent, id)
    {
        this->visible = visible;
        const auto thisPtr = this->borrowPtr<ComponentPtr>();
        this->addItemInternal(createWithImplementation<IChannel, TestChannel>(context, thisPtr, "chVis", true));
        this->addItemInternal(createWithImplementation<IChannel, TestChannel>(context, thisPtr, "chInvis", false));

        if (isRoot)
        {
            this->addItemInternal(createWithImplementation<IIoFolderConfig, TestIOFolder>(context, thisPtr, "ioVis", true, false));
            this->addItemInternal(createWithImplementation<IIoFolderConfig, TestIOFolder>(context, thisPtr, "ioInvis", false, false));
        }

        objPtr.addProperty(StringProperty("CommonProp", "DefaultValue"));
    }
};

class TestDevice : public Device
{
public:
    TestDevice(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& id, Bool visible, Bool isRoot = true)
        : Device(context, parent, id)
    {
        this->visible = visible;
        if (isRoot)
        {
            this->devices.addItem(createWithImplementation<IDevice, TestDevice>(context, this->devices, "devVis", true, false));
            this->devices.addItem(createWithImplementation<IDevice, TestDevice>(context, this->devices, "devInvis", false, false));
        }
        this->devices.addProperty(StringProperty("CommonProp", "DefaultValue"));
        this->servers.addProperty(StringProperty("CommonProp", "DefaultValue"));
        this->syncComponent.addProperty(StringProperty("CommonProp", "DefaultValue"));

        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigVis", true));
        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigInvis", false));
        this->signals.addProperty(StringProperty("CommonProp", "DefaultValue"));

        this->functionBlocks.addItem(createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, this->functionBlocks, "fbVis", true));
        this->functionBlocks.addItem(createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, this->functionBlocks, "fbInvis", false));
        this->functionBlocks.addProperty(StringProperty("CommonProp", "DefaultValue"));

        this->ioFolder.addItem(createWithImplementation<IIoFolderConfig, TestIOFolder>(context, this->ioFolder, "ioVis", true));
        this->ioFolder.addItem(createWithImplementation<IIoFolderConfig, TestIOFolder>(context, this->ioFolder, "ioInvis", false));
        this->ioFolder.addProperty(StringProperty("CommonProp", "DefaultValue"));

        objPtr.addProperty(StringProperty("DeviceProp", "DefaultValue"));
        objPtr.addProperty(StringProperty("CommonProp", "DefaultValue"));
    }

    DeviceInfoPtr onGetInfo() override
    {
        return DeviceInfo("conn");
    }
};

TEST_F(TreeTraversalTest, SubDevices)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    ASSERT_EQ(device.getDevices().getCount(), 1u);
    ASSERT_EQ(device.getDevices(Any()).getCount(), 2u);
    ASSERT_EQ(device.getDevices(Visible()).getCount(), 1u);
    ASSERT_EQ(device.getItems(Recursive(InterfaceId(IDevice::Id))).getCount(), 2u);

    const auto devices = device.getDevices(Recursive(Any()));
    const auto properties = device.findProperties(search::properties::Name("DeviceProp"), search::Recursive(search::Any()));
    ASSERT_EQ(
        device.findProperties(
                  search::properties::Name("DeviceProp"),
                  search::Recursive(search::InterfaceId(IDevice::Id))).getCount(),
        properties.getCount()
    );
    // each nested device has matching property + one property device has on its own
    ASSERT_EQ(properties.getCount(), devices.getCount() + 1u);
    for (const auto& property : properties)
    {
        ASSERT_EQ(property.getValue(), "DefaultValue");
        property.setValue("NewValue");
    }
    for (const auto& dev : devices)
    {
        ASSERT_EQ(dev.getPropertyValue("DeviceProp"), "NewValue");
    }
}

TEST_F(TreeTraversalTest, FunctionBlocks)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    ASSERT_EQ(device.getFunctionBlocks().getCount(), 1u);
    ASSERT_EQ(device.getFunctionBlocks(Any()).getCount(), 2u);
    ASSERT_EQ(device.getFunctionBlocks(Recursive(Visible())).getCount(), 2u);
    ASSERT_EQ(device.getFunctionBlocks(Recursive(Any())).getCount(), 6u);
    // 3 Devices, 2 FB per Device, 12 Ch per Device, 2 FB per Channel
    ASSERT_EQ(device.getItems(Recursive(InterfaceId(IFunctionBlock::Id))).getCount(), 3u * (2 + 12 + 12 * 2));

    const auto functionBlocks =
        device.getItems(Recursive(And(InterfaceId(IFunctionBlock::Id), Not(InterfaceId(IChannel::Id)))));
    const auto properties = device.findProperties(search::properties::Name("FunctionBlockProp"), search::Recursive(search::Any()));
    ASSERT_EQ(
        device.findProperties(
                  search::properties::Name("FunctionBlockProp"),
                  search::Recursive(search::InterfaceId(IFunctionBlock::Id))).getCount(),
        properties.getCount()
    );
    ASSERT_EQ(properties.getCount(), functionBlocks.getCount());
    for (const auto& property : properties)
    {
        ASSERT_EQ(property.getValue(), "DefaultValue");
        property.setValue("NewValue");
    }
    for (const auto& fb : functionBlocks)
    {
        ASSERT_EQ(fb.getPropertyValue("FunctionBlockProp"), "NewValue");
    }
}

TEST_F(TreeTraversalTest, Channels)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    ASSERT_EQ(device.getChannels().getCount(), 2u);
    ASSERT_EQ(device.getChannels(Any()).getCount(), 12u);
    ASSERT_EQ(device.getChannels(Recursive(Visible())).getCount(), 4u);
    ASSERT_EQ(device.getChannelsRecursive().getCount(), 4u);
    const auto channels = device.getChannels(Recursive(Any()));
    ASSERT_EQ(channels.getCount(), 36u);
    ASSERT_EQ(device.getItems(Recursive(InterfaceId(IChannel::Id))).getCount(), 36u);

    const auto properties = device.findProperties(search::properties::Name("ChannelProp"), search::Recursive(search::Any()));
    ASSERT_EQ(
        device.findProperties(
                  search::properties::Name("ChannelProp"),
                  search::Recursive(search::InterfaceId(IChannel::Id))).getCount(),
        properties.getCount()
    );
    ASSERT_EQ(properties.getCount(), channels.getCount());
    for (const auto& property : properties)
    {
        ASSERT_EQ(property.getValue(), "DefaultValue");
        property.setValue("NewValue");
    }
    for (const auto& ch : channels)
    {
        ASSERT_EQ(ch.getPropertyValue("ChannelProp"), "NewValue");
    }
}

TEST_F(TreeTraversalTest, Signals)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    ASSERT_EQ(device.getSignals().getCount(), 1u);
    ASSERT_EQ(device.getSignals(Any()).getCount(), 2u);
    ASSERT_EQ(device.getSignalsRecursive().getCount(), 12u);
    ASSERT_EQ(device.getSignals(Recursive(Visible())).getCount(), 12u);
    const auto signals = device.getSignals(Recursive(Any()));
    ASSERT_EQ(signals.getCount(), 234u);
    ASSERT_EQ(device.getItems(Recursive(InterfaceId(ISignal::Id))).getCount(), 234u);
    ASSERT_EQ(device.getItems(Recursive(LocalId("sigVis"))).getCount(), 117u);

    const auto properties = device.findProperties(search::properties::Name("SignalProp"), search::Recursive(search::Any()));
    ASSERT_EQ(
        device.findProperties(
                  search::properties::Name("SignalProp"),
                  search::Recursive(search::InterfaceId(ISignal::Id))).getCount(),
        properties.getCount()
    );
    ASSERT_EQ(properties.getCount(), signals.getCount());
    for (const auto& property : properties)
    {
        ASSERT_EQ(property.getValue(), "DefaultValue");
        property.setValue("NewValue");
    }
    for (const auto& sig : signals)
    {
        ASSERT_EQ(sig.getPropertyValue("SignalProp"), "NewValue");
    }
}

TEST_F(TreeTraversalTest, InputPorts)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    auto fb = device.getFunctionBlocks()[0];
    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
    const auto inputPorts = device.getItems(Recursive(InterfaceId(IInputPort::Id)));
    ASSERT_EQ(inputPorts.getCount(), 78u);

    const auto properties = device.findProperties(search::properties::Name("InputPortProp"), search::Recursive(search::Any()));
    ASSERT_EQ(
        device.findProperties(
                  search::properties::Name("InputPortProp"),
                  search::Recursive(search::InterfaceId(IInputPort::Id))).getCount(),
        properties.getCount()
    );
    ASSERT_EQ(properties.getCount(), inputPorts.getCount());
    for (const auto& property : properties)
    {
        ASSERT_EQ(property.getValue(), "DefaultValue");
        property.setValue("NewValue");
    }
    for (const auto& ip : inputPorts)
    {
        ASSERT_EQ(ip.getPropertyValue("InputPortProp"), "NewValue");
    }
}

TEST_F(TreeTraversalTest, SetActive)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    const auto components = device.getItems(Recursive(Any()));

    device.setActive(false);
    for (const auto& comp : components)
    {
        ASSERT_TRUE(comp.getLocalActive());
        ASSERT_FALSE(comp.getActive());
    }
    
    device.setActive(true);
    for (const auto& comp : components)
    {
        ASSERT_TRUE(comp.getLocalActive());
        ASSERT_TRUE(comp.getActive());
    }
}

TEST_F(TreeTraversalTest, IncompatibleFilters)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);

    ASSERT_THROW(device.findProperties(search::LocalId("id")), InvalidTypeException);
    ASSERT_THROW(device.findProperties(search::Any(), search::properties::Name("Name")), InvalidTypeException);
}

TEST_F(TreeTraversalTest, FindAndChangeCommonProperties)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);

    ASSERT_GT(device.findProperties(search::properties::Visible()).getCount(), 0u);
    ASSERT_GT(device.findProperties(nullptr).getCount(), 0u);

    // Default behavior: non-recursive search looks only in the device's own properties
    ASSERT_EQ(device.findProperties(search::properties::Name("CommonProp"), Any()).getCount(), 1u);
    ASSERT_EQ(device.findProperties(search::properties::Name("CommonProp")).getCount(), 1u);

    // Since the device is visible, it doesn't pass the component filter, so its properties are not searched
    ASSERT_EQ(device.findProperties(search::properties::Name("CommonProp"), Not(Visible())).getCount(), 0u);

    // No object-type properties exist, so both search variants return the same result
    ASSERT_EQ(
        device.findProperties(search::Any(), search::Recursive(search::Any())).getCount(),
        device.findProperties(search::Recursive(search::Any()), search::Recursive(search::Any())).getCount()
    );

    const auto components = device.getItems(Recursive(Any()));
    const auto properties = device.findProperties(search::properties::Name("CommonProp"), search::Recursive(search::Any()));
    // each nested component has matching property + one property device has on its own
    ASSERT_EQ(properties.getCount(), components.getCount() + 1u);
    for (const auto& property : properties)
    {
        ASSERT_EQ(property.getValue(), "DefaultValue");
        property.setValue("NewValue");
    }
    for (const auto& component : components)
    {
        ASSERT_EQ(component.getPropertyValue("CommonProp"), "NewValue");
    }
}
