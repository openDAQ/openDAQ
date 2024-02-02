#include <opendaq/signal_factory.h>
#include <opendaq/removable_ptr.h>
#include <opendaq/signal_container_impl.h>
#include <opendaq/component_ptr.h>
#include <gtest/gtest.h>
#include <opendaq/context_factory.h>
#include <opendaq/context_internal_ptr.h>
#include <coreobjects/property_object_class_factory.h>
#include <opendaq/module_manager_factory.h>
#include <coretypes/type_manager_factory.h>
#include "opendaq/mock/mock_fb_module.h"

BEGIN_NAMESPACE_OPENDAQ

class SignalContainerTest : public testing::Test
                          , public GenericSignalContainerImpl<IComponent>
{
public:
    SignalContainerTest()
        : testing::Test()
        , GenericSignalContainerImpl<IComponent>(
              Context(nullptr, Logger(), TypeManager(), ModuleManager("[[none]]")), nullptr, "localId")
    {
        
        manager = context.asPtr<IContextInternal>().moveModuleManager();
        const ModulePtr fbModule(MockFunctionBlockModule_Create(context));
        manager.addModule(fbModule);
    }

    ModuleManagerPtr manager;
};

TEST_F(SignalContainerTest, CreateWithClassName)
{
    const auto context = Context(nullptr, Logger(), TypeManager(), nullptr);

    auto rangeItemClass = PropertyObjectClassBuilder("TestClass").build();
    context.getTypeManager().addType(rangeItemClass);

    GenericSignalContainerImpl<IComponent> signalContainer(context, nullptr, "localId", "TestClass");

    StringPtr className;
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(signalContainer.getClassName(&className)));
    ASSERT_EQ(className, "TestClass");
}

TEST_F(SignalContainerTest, SignalId)
{
    auto signal = createAndAddSignal("sigId");

    ASSERT_EQ(signal.getGlobalId(), "/localId/Sig/sigId");
}

TEST_F(SignalContainerTest, RemoveSignal)
{
    auto signal = createAndAddSignal("sigId");

    removeSignal(signal);
    ASSERT_TRUE(signal.asPtr<IRemovable>(true).isRemoved());
}

TEST_F(SignalContainerTest, CreateAndAddFunctionBlock)
{
    auto fb1 = createAndAddNestedFunctionBlock("mock_fb_uid", "fb1");
    auto fb2 = createAndAddNestedFunctionBlock("mock_fb_uid", "fb2");
    auto fb3 = createAndAddNestedFunctionBlock("mock_fb_uid", "fb3");

    ASSERT_EQ(fb1.getGlobalId(), "/localId/FB/fb1");
    ASSERT_EQ(fb2.getGlobalId(), "/localId/FB/fb2");
    ASSERT_EQ(fb3.getGlobalId(), "/localId/FB/fb3");
}

TEST_F(SignalContainerTest, RemoveFunctionBlock)
{
    auto fb1 = createAndAddNestedFunctionBlock("mock_fb_uid", "fb1");
    auto fb2 = createAndAddNestedFunctionBlock("mock_fb_uid", "fb2");
    auto fb3 = createAndAddNestedFunctionBlock("mock_fb_uid", "fb3");

    removeNestedFunctionBlock(fb1);
    removeNestedFunctionBlock(fb2);
    removeNestedFunctionBlock(fb3);

    ASSERT_EQ(functionBlocks.getItems().getCount(), 0u);
}

TEST_F(SignalContainerTest, CreateAndAddFunctionBlockInvalidId)
{
    ASSERT_THROW(createAndAddNestedFunctionBlock("invalid", "fb1"), NotFoundException);
}

TEST_F(SignalContainerTest, CreateAndAddFunctionBlockDuplicateId)
{
    ASSERT_NO_THROW(createAndAddNestedFunctionBlock("mock_fb_uid", "fb1"));
    ASSERT_THROW(createAndAddNestedFunctionBlock("mock_fb_uid", "fb1"), DuplicateItemException);
    ASSERT_THROW(createAndAddNestedFunctionBlock("mock_fb_uid", "fb1"), DuplicateItemException);
}

END_NAMESPACE_OPENDAQ
