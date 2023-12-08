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
//
TEST_F(ActualInterfacesTest, ImplementationInterfaces)
{
    using ActualIntfs = ActualInterfaces<ITestObjectDerived, IUpdatable, ISerializable, ICoreType>;

    using Interfaces = ActualIntfs::BaseInterfaces;
    using Expected = Args<ITestObjectDerived,
                          IUpdatable,
                          ISerializable,
                          ICoreType,
                          ITestObject,
                          IBaseObject
                         >;

    ASSERT_TRUE((std::is_same_v<Expected, Interfaces>));
}

TEST_F(ActualInterfacesTest, ImplementationInterfacesNoDiscovery)
{
    using Interfaces = ActualInterfaces<ICoreType, ISerializable, IUpdatable>::BaseInterfaces;
    using Expected = Args<ICoreType, ISerializable, IUpdatable, IBaseObject>;

    ASSERT_TRUE((std::is_same_v<Expected, Interfaces>));
}

TEST_F(ActualInterfacesTest, Implementation)
{
    using InterfaceList = Args<ITestObjectDerived, IUpdatable, ISerializable, ICoreType, IInspectable>;

    using Interfaces = typename Meta::FoldType<InterfaceList, ActualInterfaces>::Folded::BaseInterfaces;
    using Implementation = typename Meta::FoldType<InterfaceList, IntfObjectImpl>::Folded;

    using Expected = IntfObjectImpl<ITestObjectDerived,
                                    IUpdatable,
                                    ISerializable,
                                    ICoreType,
                                    IInspectable
                                   >;

    using ImplementationIds = Implementation::InterfaceIds;
    using ExpectedInterfaces = Args<ITestObjectDerived,
                                    IUpdatable,
                                    ISerializable,
                                    ICoreType,
                                    IInspectable,
                                    ITestObject,
                                    IBaseObject>;

    ASSERT_TRUE((std::is_same_v<Expected, Implementation>));
    ASSERT_TRUE((std::is_same_v<Interfaces, ExpectedInterfaces>));
    ASSERT_TRUE((std::is_same_v<SupportsInterface<Interfaces>, ImplementationIds>));
}

TEST_F(ActualInterfacesTest, ImplementationNested)
{
    using InterfaceList = Args<ITestObjectNested, IUpdatable, ISerializable, ICoreType, IInspectable>;

    using Interfaces = typename Meta::FoldType<InterfaceList, ActualInterfaces>::Folded::BaseInterfaces;
    using Implementation = typename Meta::FoldType<InterfaceList, IntfObjectImpl>::Folded;

    using Expected = IntfObjectImpl<ITestObjectNested,
                                    IUpdatable,
                                    ISerializable,
                                    ICoreType,
                                    IInspectable
                                   >;

    using ImplementationIds = Implementation::InterfaceIds;
    using ExpectedInterfaces = Args<ITestObjectNested,
                                    IUpdatable,
                                    ISerializable,
                                    ICoreType,
                                    IInspectable,
                                    ITestObjectDerived,
                                    ITestObject,
                                    IBaseObject>;

    ASSERT_TRUE((std::is_same_v<Expected, Implementation>));
    ASSERT_TRUE((std::is_same_v<Interfaces, ExpectedInterfaces>));
    ASSERT_TRUE((std::is_same_v<SupportsInterface<Interfaces>, ImplementationIds>));
}
