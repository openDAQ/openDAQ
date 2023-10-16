#include <gtest/gtest.h>
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/arguments.h>
#include <coretypes/intfs.h>
#include <coretypes/coretype.h>
#include <coretypes/serializable.h>
#include <coretypes/updatable.h>

DECLARE_OPENDAQ_INTERFACE(ITestObject, daq::IBaseObject)
{
};

DECLARE_OPENDAQ_INTERFACE(ITestObjectDerived, ITestObject)
{
};

DECLARE_OPENDAQ_INTERFACE(ITestObjectNested, ITestObjectDerived)
{
};

using ActualInterfacesTest = testing::Test;

using namespace daq;

TEST_F(ActualInterfacesTest, HasBase)
{
    using DeclaredBase = ITestObjectDerived::Base;
    using Expected = ITestObject;

    ASSERT_TRUE((std::is_same_v<DeclaredBase, Expected>));
}

TEST_F(ActualInterfacesTest, DeclaredBase)
{
    using DeclaredBase = ITestObjectDerived::Base;
    using Expected = ITestObject;

    ASSERT_TRUE((std::is_same_v<DeclaredBase, Expected>));
}

TEST_F(ActualInterfacesTest, HasNoDeclaredBase)
{
    ASSERT_FALSE((HasDeclaredBase<IBaseObject>::value));
}

TEST_F(ActualInterfacesTest, HasDeclaredBase)
{
    ASSERT_TRUE((HasDeclaredBase<ITestObjectDerived>::value));
}

TEST_F(ActualInterfacesTest, BaseInterface)
{
    using Base = BaseInterface<ITestObjectDerived>::Interfaces;
    using Expected = Args<ITestObject, IBaseObject, IBaseObject>;

    ASSERT_TRUE((std::is_same_v<Base, Expected>));
}

TEST_F(ActualInterfacesTest, BaseInterfaceNotDeclared)
{
    using Base = BaseInterface<IBaseObject>::Interfaces;
    using Expected = Args<IBaseObject>;

    ASSERT_TRUE((std::is_same_v<Base, Expected>));
}

TEST_F(ActualInterfacesTest, ImplementationInterfaces)
{
    using ActualIntfs = ActualInterfaces<ITestObjectDerived, IUpdatable, ISerializable, ICoreType>;

    using Interfaces = ActualIntfs::Interfaces;
    using Expected = Args<ITestObjectDerived,
                          DiscoverOnly<ITestObject>,
                          IUpdatable,
                          ISerializable,
                          ICoreType,
                          IInspectable
                         >;

    ASSERT_TRUE((std::is_same_v<Expected, Interfaces>));
}

TEST_F(ActualInterfacesTest, ImplementationInterfacesNoDiscovery)
{
    using Interfaces = ActualInterfaces<ICoreType, ISerializable, IUpdatable>::Interfaces;
    using Expected = Args<ICoreType, ISerializable, IUpdatable, IInspectable>;

    ASSERT_TRUE((std::is_same_v<Expected, Interfaces>));
}

TEST_F(ActualInterfacesTest, Implementation)
{
    using Interfaces = ActualInterfaces<ITestObjectDerived, IUpdatable, ISerializable, ICoreType>::Interfaces;
    using Implementation = typename Meta::FoldType<Interfaces, IntfObjectImpl>::Folded;

    using Expected = IntfObjectImpl<ITestObjectDerived,
                                    DiscoverOnly<ITestObject>,
                                    IUpdatable,
                                    ISerializable,
                                    ICoreType,
                                    IInspectable
                                   >;

    ASSERT_TRUE((std::is_same_v<Expected, Implementation>));
}

TEST_F(ActualInterfacesTest, ImplementationNested)
{
    using Interfaces = ActualInterfaces<ITestObjectNested, IUpdatable, ISerializable, ICoreType>::Interfaces;
    using Implementation = typename Meta::FoldType<Interfaces, IntfObjectImpl>::Folded;

    using Expected = IntfObjectImpl<ITestObjectNested,
                                    DiscoverOnly<ITestObjectDerived>,
                                    DiscoverOnly<ITestObject>,
                                    IUpdatable,
                                    ISerializable,
                                    ICoreType,
                                    IInspectable
                                   >;

    ASSERT_TRUE((std::is_same_v<Expected, Implementation>));
}
