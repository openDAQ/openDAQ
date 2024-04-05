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

using TreeTraversalTest = testing::Test;
using namespace daq;
using namespace daq::search;

class TestFolder : public FolderImpl<>
{
public:
    TestFolder(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& id, Bool visible) 
        : FolderImpl<>(context, parent, id)
    {
        this->visible = visible;
    }
};

class TestComponent : public ComponentImpl<>
{
public:
    TestComponent(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& id, Bool visible) 
        : ComponentImpl<>(context, parent,id)
    {
        this->visible = visible;
    }
};

class TestSignal : public SignalImpl
{
public:
    TestSignal(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& id, Bool visible) 
        : SignalImpl(context, nullptr, parent, id)
    {
        this->visible = visible;
    }
};

class TestFunctionBlock: public FunctionBlockImpl<>
{
public:
    TestFunctionBlock(const ContextPtr& context, const ComponentPtr& parent,  const StringPtr& id, Bool visible) 
        : FunctionBlockImpl<>(nullptr, context, parent, id)
    {
        this->visible = visible;
        

        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigVis", true));
        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigInvis", false));

        this->inputPorts.addItem(InputPort(context, this->inputPorts, "ip"));
    }
};

class TestChannel : public ChannelImpl<>
{
public:
    TestChannel(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& id, Bool visible, Bool isRoot = true) 
        : ChannelImpl<>(nullptr, context, parent, id)
    {
        this->visible = visible;
        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigVis", true));
        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigInvis", false));

        this->functionBlocks.addItem(createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, this->functionBlocks, "fbVis", true));
        this->functionBlocks.addItem(createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, this->functionBlocks, "fbInvis", false));
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

        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigVis", true));
        this->signals.addItem(createWithImplementation<ISignal, TestSignal>(context, this->signals, "sigInvis", false));

        this->functionBlocks.addItem(createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, this->functionBlocks, "fbVis", true));
        this->functionBlocks.addItem(createWithImplementation<IFunctionBlock, TestFunctionBlock>(context, this->functionBlocks, "fbInvis", false));

        this->ioFolder.addItem(createWithImplementation<IIoFolderConfig, TestIOFolder>(context, this->ioFolder, "ioVis", true));
        this->ioFolder.addItem(createWithImplementation<IIoFolderConfig, TestIOFolder>(context, this->ioFolder, "ioInvis", false));
    }

    DeviceInfoPtr onGetInfo() override
    {
        auto deviceInfo = DeviceInfo("conn");
        deviceInfo.freeze();
        return deviceInfo;
    }
};

TEST_F(TreeTraversalTest, SubDevices)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    ASSERT_EQ(device.getDevices().getCount(), 1u);
    ASSERT_EQ(device.getDevices(Any()).getCount(), 2u);
    ASSERT_EQ(device.getDevices(Visible()).getCount(), 1u);
    ASSERT_EQ(device.getItems(Recursive(InterfaceId(IDevice::Id))).getCount(), 2u);
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
}

TEST_F(TreeTraversalTest, Channels)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    ASSERT_EQ(device.getChannels().getCount(), 2u);
    ASSERT_EQ(device.getChannels(Any()).getCount(), 12u);
    ASSERT_EQ(device.getChannels(Recursive(Visible())).getCount(), 4u);
    ASSERT_EQ(device.getChannelsRecursive().getCount(), 4u);
    ASSERT_EQ(device.getChannels(Recursive(Any())).getCount(), 36u);
    ASSERT_EQ(device.getItems(Recursive(InterfaceId(IChannel::Id))).getCount(), 36u);
}

TEST_F(TreeTraversalTest, Signals)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    ASSERT_EQ(device.getSignals().getCount(), 1u);
    ASSERT_EQ(device.getSignals(Any()).getCount(), 2u);
    ASSERT_EQ(device.getSignalsRecursive().getCount(), 12u);
    ASSERT_EQ(device.getSignals(Recursive(Visible())).getCount(), 12u);
    ASSERT_EQ(device.getSignals(Recursive(Any())).getCount(), 234u);
    ASSERT_EQ(device.getItems(Recursive(InterfaceId(ISignal::Id))).getCount(), 234u);
    ASSERT_EQ(device.getItems(Recursive(LocalId("sigVis"))).getCount(), 117u);
}

TEST_F(TreeTraversalTest, InputPorts)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    auto fb = device.getFunctionBlocks()[0];
    ASSERT_EQ(fb.getInputPorts().getCount(), 1u);
    ASSERT_EQ(device.getItems(Recursive(InterfaceId(IInputPort::Id))).getCount(), 78u);
}

TEST_F(TreeTraversalTest, SetActive)
{
    auto device = createWithImplementation<IDevice, TestDevice>(NullContext(), nullptr, "dev", true);
    const auto components = device.getItems(Recursive(Any()));

    device.setActive(false);
    for (const auto& comp : components)
        ASSERT_FALSE(comp.getActive());
    
    device.setActive(true);
    for (const auto& comp : components)
        ASSERT_TRUE(comp.getActive());
}
