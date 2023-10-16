#include <gtest/gtest.h>
#include <opendaq/opendaq.h>

using InterfacesTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

// Corresponding document: Antora/modules/explanation/pages/interfaces_objects_pointers.adoc
TEST_F(InterfacesTest, Interfaces)
{
    IInteger* intObj = Integer_Create(10);
    
    IConvertible* convertible;
    intObj->queryInterface(IConvertible::Id, reinterpret_cast<void**>(&convertible));
    
    Float floatVal;
    convertible->toFloat(&floatVal);
    
    IFloat* floatObj = Float_Create(10.0);
    
    IComparable* comparable;
    intObj->queryInterface(IComparable::Id, reinterpret_cast<void**>(&comparable));
    if (comparable->compareTo(floatObj) == OPENDAQ_EQUAL)
        std::cout << "equal" << std::endl;
    
    INumber* number;
    intObj->queryInterface(INumber::Id, reinterpret_cast<void**>(&number));
    
    floatObj->releaseRef();
    intObj->releaseRef();
    convertible->releaseRef();
    comparable->releaseRef();
    number->releaseRef();
}

// Corresponding document: Antora/modules/explanation/pages/interfaces_objects_pointers.adoc
TEST_F(InterfacesTest, Pointers)
{
    IntegerPtr intObj = 10;
    FloatPtr floatObj = 10.0;

    [[maybe_unused]] Float floatVal{};
    ASSERT_NO_THROW(floatVal = intObj);
    
    if (floatObj == intObj)
        std::cout << "equal" << std::endl;
    
    NumberPtr number = intObj;
}

END_NAMESPACE_OPENDAQ
