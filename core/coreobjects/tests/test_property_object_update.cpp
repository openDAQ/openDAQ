//#include <gtest/gtest.h>
//#include <coretypes/coretypes.h>
//#include <coreobjects/property_object_factory.h>
//#include <coreobjects/property_object_class_factory.h>
//#include <coreobjects/property_object_class_manager_factory.h>
//
//using namespace daq;
//
//// TODO: Fix Serialization, Deserialization, Update once rework is done
//
//class UpdateObjectTest : public testing::Test
//{
//protected:
//    void SetUp() override
//    {
//        auto toUpdateClass = PropertyObjectClass("ToUpdate");
//
//        toUpdateClass.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("IntProperty1"));
//        toUpdateClass.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("IntProperty2"));
//        toUpdateClass.addProperty(PropertyObjectClassPtr::CreateListPropInfo("ListProperty"));
//        toUpdateClass.addProperty(PropertyObjectClassPtr::CreateObjectPropInfo("ObjectProperty"));
//
//        auto dictInfo = Property();
//        dictInfo.setName("DictProperty");
//        dictinfo.setValueType(ctDict);
//        toUpdateClass.addProperty(dictInfo);
//
//        auto updateArgClass = PropertyObjectClass("UpdateArg");
//
//        updateArgClass.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("IntProperty2"));
//        updateArgClass.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("IntProperty3"));
//        updateArgClass.addProperty(PropertyObjectClassPtr::CreateObjectPropInfo("ObjectProperty"));
//
//        auto childPropClass = PropertyObjectClass("child");
//
//        childPropClass.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("PrivateInt1"));
//        childPropClass.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("PrivateInt2"));
//        childPropClass.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("PrivateInt3"));
//
//
//
//        auto manager = PropertyObjectClassManager();
//        manager.addClass(toUpdateClass);
//        manager.addClass(updateArgClass);
//        manager.addClass(childPropClass);
//    }
//
//    void TearDown() override
//    {
//        PropertyObjectClassManager().removeClass("ToUpdate");
//        PropertyObjectClassManager().removeClass("UpdateArg");
//        PropertyObjectClassManager().removeClass("child");
//    }
//};
//
//TEST_F(UpdateObjectTest, DISABLED_StaticConfigre)
//{
//    Int valueType = 1;
//    auto listType = List<Int>(1, 2, 3, 4, 5);
//
//    // PropertyObject
//
//    auto childObj = PropertyObject("child");
//    childObj.setPropertyValue("PrivateInt1", 1);
//    childObj.setPropertyValue("PrivateInt2", 4);
//
//    PropertyObjectPtr objToUpdate = PropertyObject("ToUpdate");
//    objToUpdate.setPropertyValue("IntProperty1", 1);
//    objToUpdate.setPropertyValue("IntProperty2", 1);
//    objToUpdate.setPropertyValue("ListProperty", listType);
//    objToUpdate.setPropertyValue("ObjectProperty", childObj);
//
//    DictPtr<IString, IBaseObject> dictType = Dict<IString, IBaseObject>();
//    dictType.set("test", valueType);
//    dictType.set("test2", listType);
//
//    objToUpdate.setPropertyValue("DictProperty", dictType);
//
//    // Update object
//
//    auto childObjUpdate = PropertyObject("child");
//    childObjUpdate.setPropertyValue("PrivateInt2", 2);
//    childObjUpdate.setPropertyValue("PrivateInt3", 3);
//
//    PropertyObjectPtr updateObj = PropertyObject("UpdateArg");
//    updateObj.setPropertyValue("IntProperty2", 2);
//    updateObj.setPropertyValue("IntProperty3", 3);
//    updateObj.setPropertyValue("ObjectProperty", childObjUpdate);
//
//    SerializerPtr serializer = JsonSerializer();
//    updateObj.serialize(serializer);
//
//    StringPtr updateJson = serializer.getOutput();
//
//    // Perform the update
//
//    DeserializerPtr deserializer = JsonDeserializer();
//    deserializer.update(objToUpdate, ConfigurationMode::Static, updateJson);
//
//    Int value = objToUpdate.getPropertyValue("IntProperty2");
//    ASSERT_EQ(value, 2);
//
//    // Property in JSON but not in PropObject should be skipped
//    ASSERT_FALSE(objToUpdate.hasProperty("IntProperty3"));
//
//    // Property not in JSON but set in PropObject should skipped
//    ASSERT_TRUE(objToUpdate.hasProperty("ListProperty"));
//    ASSERT_TRUE(objToUpdate.hasProperty("DictProperty"));
//    ASSERT_TRUE(objToUpdate.hasProperty("ObjectProperty"));
//
//    GenericPropertyObjectPtr propChild = objToUpdate.getPropertyValue("ObjectProperty");
//
//    // Ignore new property in JSON
//    ASSERT_TRUE(propChild.hasProperty("PrivateInt3"));
//
//    // Update existing property
//    ASSERT_TRUE(propChild.hasProperty("PrivateInt1"));
//    ASSERT_EQ(propChild.getPropertyValue("PrivateInt2"), 2);
//}
//
//TEST_F(UpdateObjectTest, DISABLED_UpdateFrozen)
//{
//    Int valueType = 1;
//    auto listType = List<Int>(1, 2, 3, 4, 5);
//
//    // PropertyObject
//
//    auto childObj = PropertyObject("child");
//    childObj.setPropertyValue("privateValue", 1);
//
//    PropertyObjectPtr objToUpdate = PropertyObject("test");
//    objToUpdate.setPropertyValue("valueType", valueType);
//    objToUpdate.setPropertyValue("listType", listType);
//    objToUpdate.setPropertyValue("child", childObj);
//
//    objToUpdate.freeze();
//
//    SerializerPtr serializer = JsonSerializer();
//    childObj.serialize(serializer);
//
//    StringPtr updateJson = serializer.getOutput();
//
//    // Perform the update
//
//    DeserializerPtr deserializer = JsonDeserializer();
//    ErrCode errCode = deserializer->update(objToUpdate.asPtr<IUpdatable>(true), ConfigurationMode::Static, updateJson);
//    ASSERT_EQ(errCode, OPENDAQ_IGNORED);
//}
//
//TEST_F(UpdateObjectTest, DISABLED_StaticConfigreWithChildObject)
//{
//    Int valueType = 1;
//    auto listType = List<Int>(1, 2, 3, 4, 5);
//
//    // PropertyObject
//
//    auto childObj = PropertyObject("child");
//    childObj.setPropertyValue("privateValue", 1);
//
//    PropertyObjectPtr objToUpdate = PropertyObject("test");
//    objToUpdate.setPropertyValue("valueType", 1);
//    objToUpdate.setPropertyValue("listType", listType);
//    objToUpdate.setPropertyValue("child", childObj);
//
//    DictPtr<IString, IBaseObject> dictType = Dict<IString, IBaseObject>();
//    dictType.set("test", valueType);
//    dictType.set("test2", listType);
//
//    objToUpdate.setPropertyValue("dictType", dictType);
//
//    // Update object
//
//    auto childObjUpdate = PropertyObject("child");
//    childObjUpdate.setPropertyValue("privateValue", 2);
//    childObjUpdate.setPropertyValue("privateValue3", 3);
//
//    PropertyObjectPtr updateObj = PropertyObject("test");
//    updateObj.setPropertyValue("valueType", 2);
//    updateObj.setPropertyValue("valueType3", 3);
//    updateObj.setPropertyValue("child", childObjUpdate);
//
//    SerializerPtr serializer = JsonSerializer();
//    updateObj.serialize(serializer);
//
//    StringPtr updateJson = serializer.getOutput();
//
//    // Perform the update
//
//    DeserializerPtr deserializer = JsonDeserializer();
//    deserializer.update(objToUpdate, ConfigurationMode::Static, updateJson);
//
//    Int value = objToUpdate.getPropertyValue("valueType");
//    ASSERT_EQ(value, 2);
//
//    // Property in JSON but not in PropObject should be skipped
//    ASSERT_FALSE(objToUpdate.hasProperty("valueType3"));
//
//    // Property not in JSON but set in PropObject should cleared
//    ASSERT_TRUE(objToUpdate.hasProperty("listType"));
//    ASSERT_TRUE(objToUpdate.hasProperty("dictType"));
//
//    ASSERT_TRUE(objToUpdate.hasProperty("child"));
//
//    GenericPropertyObjectPtr obj = objToUpdate.getPropertyValueOrNull("child");
//    Int privateValue = obj.getPropertyValueOrNull("privateValue");
//
//    ASSERT_EQ(privateValue, 2);
//    ASSERT_EQ(childObj.getPropertyValue("privateValue"), 2);
//}
//
//TEST_F(UpdateObjectTest, DISABLED_DynamicConfigure)
//{
//    Int valueType = 1;
//    auto listType = List<Int>(1, 2, 3, 4, 5);
//
//    // PropertyObject
//
//    auto childObj = PropertyObject("child");
//    childObj.setPropertyValue("privateValue", 1);
//
//    PropertyObjectPtr objToUpdate = PropertyObject("test");
//    objToUpdate.setPropertyValue("valueType", 1);
//    objToUpdate.setPropertyValue("listType", listType);
//    objToUpdate.setPropertyValue("child", childObj);
//
//    DictPtr<IString, IBaseObject> dictType = Dict<IString, IBaseObject>();
//    dictType.set("test", valueType);
//    dictType.set("test2", listType);
//
//    objToUpdate.setPropertyValue("dictType", dictType);
//
//    // Update object
//
//    auto childObjUpdate = PropertyObject("child");
//    childObjUpdate.setPropertyValue("privateValue", 2);
//    childObjUpdate.setPropertyValue("privateValue3", 3);
//
//    PropertyObjectPtr updateObj = PropertyObject("test");
//    updateObj.setPropertyValue("valueType", 2);
//    updateObj.setPropertyValue("valueType3", 3);
//    updateObj.setPropertyValue("child", childObjUpdate);
//
//    SerializerPtr serializer = JsonSerializer();
//    updateObj.serialize(serializer);
//
//    StringPtr updateJson = serializer.getOutput();
//
//    // Perform the update
//
//    DeserializerPtr deserializer = JsonDeserializer();
//    deserializer.update(objToUpdate, ConfigurationMode::Dynamic, updateJson);
//
//    Int value = objToUpdate.getPropertyValue("valueType");
//    ASSERT_EQ(value, 2);
//
//    // Property in JSON but not in PropObject should be created & updated
//    ASSERT_TRUE(objToUpdate.hasProperty("valueType3"));
//    ASSERT_EQ(objToUpdate.getPropertyValue("valueType3"), 3);
//
//    // Property not in JSON but set in PropObject should cleared
//    ASSERT_FALSE(objToUpdate.hasProperty("listType"));
//    ASSERT_FALSE(objToUpdate.hasProperty("dictType"));
//
//    ASSERT_TRUE(objToUpdate.hasProperty("child"));
//
//    GenericPropertyObjectPtr obj = objToUpdate.getPropertyValueOrNull("child");
//
//    // updates property object without recreating it
//    ASSERT_EQ(childObj.getPropertyValue("privateValue"), 2);
//    ASSERT_EQ(childObj.getPropertyValue("privateValue"), obj.getPropertyValueOrNull("privateValue"));
//
//    // New property created from the serialized object
//    ASSERT_TRUE(obj.hasProperty("privateValue3"));
//    ASSERT_EQ(obj.getPropertyValue("privateValue3"), 3);
//}
//
//TEST_F(UpdateObjectTest, DISABLED_StaticUpdateWithList)
//{
//    auto elementObj = PropertyObject("test");
//    elementObj.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("IsTest", 1));
//    elementObj.setPropertyValue("IsTest", 2);
//    elementObj.setPropertyValue("IsTest2", 5.6);
//
//    auto listType = List<IBaseObject>(1, 2.6, True, 4, elementObj);
//
//    // PropertyObject
//
//    auto childObj = PropertyObject("child");
//    // childObj.setPropertyValue("privateValue", 1);
//
//    PropertyObjectPtr objToUpdate = PropertyObject("test");
//    objToUpdate.setPropertyValue("listType", listType);
//
//    // Update object
//
//    auto elementUpdateObj = PropertyObject("test");
//    elementUpdateObj.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("IsTest", 1));
//    elementUpdateObj.setPropertyValue("IsTest", 3);
//    elementUpdateObj.setPropertyValue("IsTest2", 6.6);
//
//    auto listTypeUpdate = List<IBaseObject>(2, 3.6, False, 5, elementUpdateObj);
//
//    PropertyObjectPtr updateObj = PropertyObject("test");
//    updateObj.setPropertyValue("listType", listTypeUpdate);
//
//    SerializerPtr serializer = JsonSerializer();
//    updateObj.serialize(serializer);
//
//    StringPtr updateJson = serializer.getOutput();
//
//    // Perform the update
//
//    DeserializerPtr deserializer = JsonDeserializer();
//    deserializer.update(objToUpdate, ConfigurationMode::Static, updateJson);
//
//    ListPtr<IBaseObject> updatedList = objToUpdate.getPropertyValue("listType");
//    ASSERT_EQ(updatedList[0], 2);
//    ASSERT_DOUBLE_EQ(updatedList[1], 3.6);
//    ASSERT_EQ(updatedList[2], False);
//    ASSERT_EQ(updatedList[3], 5);
//
//    GenericPropertyObjectPtr updatedElementObj = updatedList[4];
//    ASSERT_EQ(updatedElementObj.getPropertyValue("IsTest"), 3);
//    ASSERT_DOUBLE_EQ(updatedElementObj.getPropertyValue("IsTest2"), 6.6);
//}
//
//TEST_F(UpdateObjectTest, DISABLED_StaticUpdateWithNestedList)
//{
//    auto elementObj = PropertyObject("test");
//    elementObj.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("IsTest", 1));
//    elementObj.setPropertyValue("IsTest", 2);
//    elementObj.setPropertyValue("IsTest2", 5.6);
//
//    auto listType = List<IBaseObject>(1, 2.6, True, List<Int>(1, 2), elementObj);
//
//    // PropertyObject
//
//    PropertyObjectPtr objToUpdate = PropertyObject("test");
//    objToUpdate.setPropertyValue("listType", listType);
//
//    // Update object
//
//    auto elementUpdateObj = PropertyObject("test");
//    elementUpdateObj.addProperty(PropertyObjectClassPtr::CreateIntPropInfo("IsTest", 1));
//    elementUpdateObj.setPropertyValue("IsTest", 3);
//    elementUpdateObj.setPropertyValue("IsTest2", 6.6);
//
//    auto listTypeUpdate = List<IBaseObject>(2, 3.6, False, List<Int>(3, 4), elementUpdateObj);
//
//    PropertyObjectPtr updateObj = PropertyObject("test");
//    updateObj.setPropertyValue("listType", listTypeUpdate);
//
//    SerializerPtr serializer = JsonSerializer();
//    updateObj.serialize(serializer);
//
//    StringPtr updateJson = serializer.getOutput();
//
//    // Perform the update
//
//    DeserializerPtr deserializer = JsonDeserializer();
//    deserializer.update(objToUpdate, ConfigurationMode::Static, updateJson);
//
//    ListPtr<IBaseObject> updatedList = objToUpdate.getPropertyValue("listType");
//    ASSERT_EQ(updatedList[0], 2);
//    ASSERT_DOUBLE_EQ(updatedList[1], 3.6);
//    ASSERT_EQ(updatedList[2], False);
//
//    ListPtr<IInteger> updatedNestedList = updatedList[3];
//    ASSERT_EQ(updatedNestedList[0], 3);
//    ASSERT_EQ(updatedNestedList[1], 4);
//
//    GenericPropertyObjectPtr updatedElementObj = updatedList[4];
//    ASSERT_EQ(updatedElementObj.getPropertyValue("IsTest"), 3);
//    ASSERT_DOUBLE_EQ(updatedElementObj.getPropertyValue("IsTest2"), 6.6);
//}
//
//TEST_F(UpdateObjectTest, DISABLED_StaticUpdateWithPropertyChangedInNestedEvent)
//{
//    // Naming is important - default order is used!
//    const char* triggerProperty = "Test1";
//    const char* defaultProperty = "Test2";
//
//    auto buildTestObject = [&] {
//        auto testObj = PropertyObject("test");
//        auto eventProperty = PropertyObjectClassPtr::CreateIntPropInfo(triggerProperty, 0);
//        eventProperty.getOnValueChanged() += [&](GenericPropertyObjectPtr& obj, PropertyValueEventArgsPtr& /*args*/) {
//            obj.setPropertyValue(defaultProperty, Ratio(10000000, 1));
//        };
//        testObj.addProperty(eventProperty);
//        testObj.addProperty(PropertyObjectClassPtr::CreatePropInfo(defaultProperty, ctRatio, Ratio(15000000, 1)));
//        return testObj;
//    };
//
//    //Contruct object and serialize
//    auto sourceObj = buildTestObject();
//    sourceObj.setPropertyValue(triggerProperty, 1);
//
//    SerializerPtr serializer = JsonSerializer();
//    sourceObj.serialize(serializer);
//    StringPtr updateJson = serializer.getOutput();
//
//    // Perform the update
//    auto targetObj = buildTestObject();
//    DeserializerPtr deserializer = JsonDeserializer();
//    deserializer.update(targetObj, ConfigurationMode::Static, updateJson);
//
//    ASSERT_EQ(10000000, (int) targetObj.getPropertyValue(defaultProperty));
//    ASSERT_EQ(1, (int) targetObj.getPropertyValue(triggerProperty));
//}
//
//TEST_F(UpdateObjectTest, DISABLED_UpdateLocalStringPropertyAfterDictPropertyWithProperty)
//{
//    GenericPropertyObjectPtr genericPropObject = PropertyObject(nullptr);
//
//    genericPropObject.addProperty(PropertyObjectClassPtr::CreatePropInfo("DictProp", ctDict));
//    genericPropObject.setPropertyValue("LocalStringProp", "test");
//
//    // 'Dict' value must be serialized before 'LocalStringProp'
//    StringPtr updateJson = R"({"__type":"PropertyObject","propValues":{"DictProp":{"__type":"Dict","values":[]},"LocalStringProp":"test"}})";
//
//    DeserializerPtr deserializer = JsonDeserializer();
//    ASSERT_NO_THROW(deserializer.update(genericPropObject, ConfigurationMode::Static, updateJson));
//}
