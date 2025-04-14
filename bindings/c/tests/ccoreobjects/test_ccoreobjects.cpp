#include <copendaq.h>

#include <gtest/gtest.h>

using CCoreobjectsTest = testing::Test;

TEST_F(CCoreobjectsTest, ArgumentInfo)
{
    ArgumentInfo* argInfo = nullptr;
    String* name = nullptr;
    String_createString(&name, "test_argument");
    ErrCode err = 0;
    err = ArgumentInfo_createArgumentInfo(&argInfo, name, CoreType::ctInt);

    String* name2 = nullptr;
    CoreType type = CoreType::ctUndefined;
    err = ArgumentInfo_getName(argInfo, &name2);
    ASSERT_EQ(err, 0);
    err = ArgumentInfo_getType(argInfo, &type);
    ASSERT_EQ(err, 0);
    ConstCharPtr str = nullptr;
    String_getCharPtr(name2, &str);
    ASSERT_STREQ(str, "test_argument");
    ASSERT_EQ(type, CoreType::ctInt);

    BaseObject_releaseRef(name2);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(argInfo);
}

TEST_F(CCoreobjectsTest, AuthenticationProvider)
{
    AuthenticationProvider* authProvider = nullptr;
    ErrCode err = 0;
    User* user = nullptr;
    String* username = nullptr;
    String* passwordHash = nullptr;
    List* groups = nullptr;
    String_createString(&username, "test_user");
    String_createString(&passwordHash, "test_hash");
    List_createList(&groups);
    User_createUser(&user, username, passwordHash, groups);

    List* userList = nullptr;
    List_createList(&userList);
    List_pushBack(userList, user);
    err = AuthenticationProvider_createStaticAuthenticationProvider(&authProvider, True, userList);
    ASSERT_EQ(err, 0);

    User* userOut = nullptr;
    err = AuthenticationProvider_authenticateAnonymous(authProvider, &userOut);
    ASSERT_EQ(err, 0);
    ASSERT_NE(userOut, nullptr);
    BaseObject_releaseRef(userOut);
    userOut = nullptr;

    err = AuthenticationProvider_authenticate(authProvider, username, passwordHash, &userOut);
    ASSERT_EQ(err, 0);
    ASSERT_NE(userOut, nullptr);
    BaseObject_releaseRef(userOut);
    userOut = nullptr;

    err = AuthenticationProvider_findUser(authProvider, username, &userOut);
    ASSERT_EQ(err, 0);
    ASSERT_NE(userOut, nullptr);
    BaseObject_releaseRef(userOut);

    BaseObject_releaseRef(userList);
    BaseObject_releaseRef(groups);
    BaseObject_releaseRef(user);
    BaseObject_releaseRef(passwordHash);
    BaseObject_releaseRef(username);
    BaseObject_releaseRef(authProvider);
}

TEST_F(CCoreobjectsTest, CallableInfo)
{
    CallableInfo* callableInfo = nullptr;
    ErrCode err = 0;

    List* argumentInfo = nullptr;
    List_createList(&argumentInfo);

    String* name = nullptr;
    String_createString(&name, "test_argument");
    ArgumentInfo* argInfo = nullptr;
    ArgumentInfo_createArgumentInfo(&argInfo, name, CoreType::ctInt);
    List_pushBack(argumentInfo, argInfo);

    err = CallableInfo_createCallableInfo(&callableInfo, argumentInfo, CoreType::ctInt, True);
    ASSERT_EQ(err, 0);

    Bool isConst = False;
    CoreType returnType = CoreType::ctUndefined;
    List* arguments = nullptr;

    CallableInfo_isConst(callableInfo, &isConst);
    CallableInfo_getReturnType(callableInfo, &returnType);
    CallableInfo_getArguments(callableInfo, &arguments);

    ASSERT_EQ(isConst, True);
    ASSERT_EQ(returnType, CoreType::ctInt);
    ASSERT_NE(arguments, nullptr);
    SizeT size = 0;
    List_getCount(arguments, &size);
    ASSERT_EQ(size, 1);

    BaseObject_releaseRef(arguments);
    BaseObject_releaseRef(argInfo);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(argumentInfo);
    BaseObject_releaseRef(callableInfo);
}

TEST_F(CCoreobjectsTest, Coercer)
{
    Coercer* coercer = nullptr;
    ErrCode err = 0;
    String* evalStr = nullptr;
    String_createString(&evalStr, "value + 2");
    err = Coercer_createCoercer(&coercer, evalStr);
    ASSERT_EQ(err, 0);
    ASSERT_NE(coercer, nullptr);
    Integer* value = nullptr;
    Integer_createInteger(&value, 10);
    Integer* coercedValue = nullptr;
    Coercer_coerce(coercer, nullptr, value, (BaseObject**) &coercedValue);
    ASSERT_NE(coercedValue, nullptr);
    Int coercedInt = 0;
    Integer_getValue(coercedValue, &coercedInt);
    ASSERT_EQ(coercedInt, 12);

    BaseObject_releaseRef(coercedValue);
    BaseObject_releaseRef(value);
    BaseObject_releaseRef(evalStr);
    BaseObject_releaseRef(coercer);
}

TEST_F(CCoreobjectsTest, CoreEventArgs)
{
}

static Bool eventCalled = False;
static void onPropertyObjectUpdateEnd(BaseObject* sender, BaseObject* args)
{
    EndUpdateEventArgs* eventArgs = (EndUpdateEventArgs*) args;
    List* properties = nullptr;
    EndUpdateEventArgs_getProperties(eventArgs, &properties);
    SizeT count = 0;
    List_getCount(properties, &count);
    if (count == 0)
        eventCalled = True;

    BaseObject_releaseRef(properties);
    BaseObject_releaseRef(sender);
    BaseObject_releaseRef(args);
}

TEST_F(CCoreobjectsTest, EndUpdateEventArgs)
{
    PropertyObject* propObj = nullptr;
    PropertyObject_createPropertyObject(&propObj);

    Event* event = nullptr;
    PropertyObject_getOnEndUpdate(propObj, &event);

    EventHandler* handler = nullptr;
    EventHandler_createEventHandler(&handler, onPropertyObjectUpdateEnd);

    Event_addHandler(event, handler);

    PropertyObject_beginUpdate(propObj);
    PropertyObject_endUpdate(propObj);
    ASSERT_EQ(eventCalled, True);

    BaseObject_releaseRef(handler);
    BaseObject_releaseRef(event);
    BaseObject_releaseRef(propObj);
}

TEST_F(CCoreobjectsTest, EvalValue)
{
    PropertyObject* propObj = nullptr;
    PropertyObject_createPropertyObject(&propObj);

    String* name = nullptr;
    String_createString(&name, "test_property");
    Integer* defaultValue = nullptr;
    Integer_createInteger(&defaultValue, 10);
    Boolean* visible = nullptr;
    Boolean_createBoolean(&visible, True);
    Property* prop = nullptr;
    Property_createIntProperty(&prop, name, defaultValue, visible);

    PropertyObject_addProperty(propObj, prop);

    String* refName = nullptr;
    String_createString(&refName, "ref_property");
    String* evalStr = nullptr;
    String_createString(&evalStr, "%test_property");
    EvalValue* evalValue = nullptr;
    EvalValue_createEvalValue(&evalValue, evalStr);
    Property* refProp = nullptr;
    Property_createReferenceProperty(&refProp, refName, evalValue);

    PropertyObject_addProperty(propObj, refProp);

    Integer* value = nullptr;
    PropertyObject_getPropertyValue(propObj, refName, (BaseObject**) &value);
    ASSERT_NE(value, nullptr);
    Int intValue = 0;
    Integer_getValue(value, &intValue);
    ASSERT_EQ(intValue, 10);

    BaseObject_releaseRef(value);
    BaseObject_releaseRef(refName);
    BaseObject_releaseRef(evalStr);
    BaseObject_releaseRef(evalValue);
    BaseObject_releaseRef(refProp);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(defaultValue);
    BaseObject_releaseRef(visible);
    BaseObject_releaseRef(prop);
    BaseObject_releaseRef(propObj);
}

TEST_F(CCoreobjectsTest, Ownable)
{
    ErrCode err = 0;
    PropertyObject* propObj = nullptr;
    PropertyObject_createPropertyObject(&propObj);
    PropertyObject* parentObj = nullptr;
    PropertyObject_createPropertyObject(&parentObj);

    Ownable* ownable = nullptr;
    BaseObject_borrowInterface(propObj, OWNABLE_INTF_ID, (BaseObject**) &ownable);

    err = Ownable_setOwner(ownable, parentObj);
    ASSERT_EQ(err, 0);
    err = Ownable_setOwner(ownable, nullptr);
    ASSERT_EQ(err, 0);

    BaseObject_releaseRef(ownable);
    BaseObject_releaseRef(parentObj);
}

TEST_F(CCoreobjectsTest, Permissions)
{
    List* adminGroups = nullptr;
    List_createList(&adminGroups);
    List* guestGroups = nullptr;
    List_createList(&guestGroups);

    String* adminName = nullptr;
    String_createString(&adminName, "admin");
    String* guestName = nullptr;
    String_createString(&guestName, "guest");
    String* password = nullptr;
    String_createString(&password, "password");

    List_pushBack(adminGroups, adminName);
    List_pushBack(adminGroups, guestName);
    List_pushBack(guestGroups, guestName);

    User* admin = nullptr;
    User_createUser(&admin, adminName, password, adminGroups);
    User* guest = nullptr;
    User_createUser(&guest, guestName, password, guestGroups);

    PermissionManager* manager = nullptr;
    PermissionManager_createPermissionManager(&manager, nullptr);

    PermissionMaskBuilder* maskBuilder = nullptr;
    PermissionMaskBuilder_createPermissionMaskBuilder(&maskBuilder);
    PermissionMaskBuilder_read(maskBuilder);
    PermissionMaskBuilder_write(maskBuilder);

    PermissionsBuilder* permissionsBuilder = nullptr;
    PermissionsBuilder_createPermissionsBuilder(&permissionsBuilder);
    PermissionsBuilder_assign(permissionsBuilder, adminName, maskBuilder);
    Permissions* adminPermissions = nullptr;
    PermissionsBuilder_build(permissionsBuilder, &adminPermissions);

    PermissionManager_setPermissions(manager, adminPermissions);
    Bool isAuthorized = False;
    PermissionManager_isAuthorized(manager, admin, Permission::PermissionRead, &isAuthorized);
    ASSERT_EQ(isAuthorized, True);
    isAuthorized = False;
    PermissionManager_isAuthorized(manager, admin, Permission::PermissionWrite, &isAuthorized);
    ASSERT_EQ(isAuthorized, True);
    isAuthorized = False;
    PermissionManager_isAuthorized(manager, admin, Permission::PermissionExecute, &isAuthorized);
    ASSERT_EQ(isAuthorized, False);

    isAuthorized = False;
    PermissionManager_isAuthorized(manager, guest, Permission::PermissionRead, &isAuthorized);
    ASSERT_EQ(isAuthorized, False);
    isAuthorized = False;
    PermissionManager_isAuthorized(manager, guest, Permission::PermissionWrite, &isAuthorized);
    ASSERT_EQ(isAuthorized, False);
    isAuthorized = False;
    PermissionManager_isAuthorized(manager, guest, Permission::PermissionExecute, &isAuthorized);
    ASSERT_EQ(isAuthorized, False);

    BaseObject_releaseRef(manager);
    BaseObject_releaseRef(adminPermissions);
    BaseObject_releaseRef(permissionsBuilder);
    BaseObject_releaseRef(maskBuilder);
    BaseObject_releaseRef(admin);
    BaseObject_releaseRef(guest);
    BaseObject_releaseRef(adminName);
    BaseObject_releaseRef(guestName);
    BaseObject_releaseRef(password);
    BaseObject_releaseRef(adminGroups);
    BaseObject_releaseRef(guestGroups);
}

TEST_F(CCoreobjectsTest, Property)
{
    Property* prop = nullptr;
    ErrCode err = 0;
    String* name = nullptr;
    String_createString(&name, "test_property");
    Integer* defaultValue = nullptr;
    Integer_createInteger(&defaultValue, 10);
    Boolean* visible = nullptr;
    Boolean_createBoolean(&visible, True);
    err = Property_createIntProperty(&prop, name, defaultValue, visible);
    ASSERT_EQ(err, 0);
    Integer* defaultValueOut = nullptr;
    Property_getDefaultValue(prop, (BaseObject**) &defaultValueOut);
    ASSERT_NE(defaultValueOut, nullptr);
    Int value = 0;
    err = Integer_getValue(defaultValueOut, &value);
    ASSERT_EQ(value, 10);

    String* nameOut = nullptr;
    Property_getName(prop, &nameOut);
    ConstCharPtr str = nullptr;
    String_getCharPtr(nameOut, &str);
    ASSERT_STREQ(str, "test_property");

    Bool isVisible = False;
    Property_getVisible(prop, &isVisible);
    ASSERT_EQ(isVisible, True);

    BaseObject_releaseRef(nameOut);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(defaultValueOut);
    BaseObject_releaseRef(defaultValue);
    BaseObject_releaseRef(visible);
    BaseObject_releaseRef(prop);
}

TEST_F(CCoreobjectsTest, PropertyBuilder)
{
    PropertyBuilder* propBuilder = nullptr;
    ErrCode err = 0;

    String* name = nullptr;
    String_createString(&name, "test_property");
    Integer* defaultValue = nullptr;
    Integer_createInteger(&defaultValue, 10);
    Boolean* visible = nullptr;
    Boolean_createBoolean(&visible, True);
    err = PropertyBuilder_createIntPropertyBuilder(&propBuilder, name, defaultValue);
    ASSERT_EQ(err, 0);
    err = PropertyBuilder_setVisible(propBuilder, visible);
    ASSERT_EQ(err, 0);
    Property* property = nullptr;
    err = PropertyBuilder_build(propBuilder, &property);
    ASSERT_EQ(err, 0);

    Integer* defaultValueOut = nullptr;
    Property_getDefaultValue(property, (BaseObject**) &defaultValueOut);
    ASSERT_NE(defaultValueOut, nullptr);
    Int value = 0;
    err = Integer_getValue(defaultValueOut, &value);
    ASSERT_EQ(value, 10);

    String* nameOut = nullptr;
    Property_getName(property, &nameOut);
    ConstCharPtr str = nullptr;
    String_getCharPtr(nameOut, &str);
    ASSERT_STREQ(str, "test_property");

    Bool isVisible = False;
    Property_getVisible(property, &isVisible);
    ASSERT_EQ(isVisible, True);

    BaseObject_releaseRef(nameOut);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(defaultValueOut);
    BaseObject_releaseRef(defaultValue);
    BaseObject_releaseRef(visible);
    BaseObject_releaseRef(property);
    BaseObject_releaseRef(propBuilder);
}

TEST_F(CCoreobjectsTest, PropertyObject)
{
    PropertyObject* propObj = nullptr;
    ErrCode err = 0;

    PropertyObject_createPropertyObject(&propObj);
    ASSERT_NE(propObj, nullptr);

    Property* prop = nullptr;
    String* name = nullptr;
    String_createString(&name, "test_property");
    Integer* defaultValue = nullptr;
    Integer_createInteger(&defaultValue, 10);
    Boolean* visible = nullptr;
    Boolean_createBoolean(&visible, True);
    err = Property_createIntProperty(&prop, name, defaultValue, visible);
    ASSERT_EQ(err, 0);
    err = PropertyObject_addProperty(propObj, prop);
    ASSERT_EQ(err, 0);

    Property* propOut = nullptr;
    err = PropertyObject_getProperty(propObj, name, &propOut);
    ASSERT_EQ(err, 0);
    ASSERT_NE(propOut, nullptr);

    Bool equal = False;
    err = BaseObject_equals(prop, propOut, &equal);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(equal, True);

    err = PropertyObject_hasProperty(propObj, name, &equal);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(equal, True);

    err = PropertyObject_removeProperty(propObj, name);
    ASSERT_EQ(err, 0);

    err = PropertyObject_hasProperty(propObj, name, &equal);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(equal, False);

    BaseObject_releaseRef(propOut);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(defaultValue);
    BaseObject_releaseRef(visible);
    BaseObject_releaseRef(prop);
    BaseObject_releaseRef(propObj);
}

TEST_F(CCoreobjectsTest, PropertyObjectClass)
{
    PropertyObjectClass* propObjClass = nullptr;
    PropertyObjectClassBuilder* builder = nullptr;
    ErrCode err = 0;
    String* name = nullptr;
    String_createString(&name, "test_property_class");

    err = PropertyObjectClassBuilder_createPropertyObjectClassBuilder(&builder, name);
    ASSERT_EQ(err, 0);
    ASSERT_NE(builder, nullptr);

    Property* prop = nullptr;
    String* propName = nullptr;
    String_createString(&propName, "test_property");
    Integer* defaultValue = nullptr;
    Integer_createInteger(&defaultValue, 10);
    Boolean* visible = nullptr;
    Boolean_createBoolean(&visible, True);
    err = Property_createIntProperty(&prop, propName, defaultValue, visible);
    ASSERT_EQ(err, 0);

    err = PropertyObjectClassBuilder_addProperty(builder, prop);
    ASSERT_EQ(err, 0);

    err = PropertyObjectClassBuilder_build(builder, &propObjClass);
    ASSERT_EQ(err, 0);

    Property* propOut = nullptr;
    err = PropertyObjectClass_getProperty(propObjClass, propName, &propOut);
    ASSERT_EQ(err, 0);
    ASSERT_NE(propOut, nullptr);

    Bool equal = False;
    err = BaseObject_equals(prop, propOut, &equal);
    ASSERT_EQ(err, 0);
    ASSERT_EQ(equal, True);

    BaseObject_releaseRef(propOut);
    BaseObject_releaseRef(propName);
    BaseObject_releaseRef(defaultValue);
    BaseObject_releaseRef(visible);
    BaseObject_releaseRef(prop);
    BaseObject_releaseRef(propObjClass);
    BaseObject_releaseRef(builder);
    BaseObject_releaseRef(name);
}

TEST_F(CCoreobjectsTest, PropertyObjectProtected)
{
    ErrCode err = 0;
    PropertyObject* propObj = nullptr;
    PropertyObjectProtected* propObjProtected = nullptr;
    PropertyObject_createPropertyObject(&propObj);
    BaseObject_borrowInterface(propObj, PROPERTY_OBJECT_PROTECTED_INTF_ID, (BaseObject**) &propObjProtected);

    PropertyBuilder* propBuilder = nullptr;
    String* name = nullptr;
    String_createString(&name, "test_property");
    Integer* defaultValue = nullptr;
    Integer_createInteger(&defaultValue, 10);
    Boolean* readOnly = nullptr;
    Boolean_createBoolean(&readOnly, True);
    PropertyBuilder_createIntPropertyBuilder(&propBuilder, name, defaultValue);
    PropertyBuilder_setReadOnly(propBuilder, readOnly);
    Property* prop = nullptr;
    PropertyBuilder_build(propBuilder, &prop);
    PropertyObject_addProperty(propObj, prop);
    Integer* value = nullptr;
    Integer_createInteger(&value, 20);

    err = PropertyObject_setPropertyValue(propObj, name, value);
    ASSERT_NE(err, 0);

    err = PropertyObjectProtected_setProtectedPropertyValue(propObjProtected, name, value);
    ASSERT_EQ(err, 0);
    Integer* valueOut = nullptr;
    PropertyObject_getPropertyValue(propObj, name, (BaseObject**) &valueOut);
    ASSERT_NE(valueOut, nullptr);
    Int intValue = 0;
    Integer_getValue(valueOut, &intValue);
    ASSERT_EQ(intValue, 20);

    BaseObject_releaseRef(valueOut);
    BaseObject_releaseRef(value);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(defaultValue);
    BaseObject_releaseRef(readOnly);
    BaseObject_releaseRef(prop);
    BaseObject_releaseRef(propBuilder);
    BaseObject_releaseRef(propObjProtected);
}

TEST_F(CCoreobjectsTest, PropertyValueEventArgs)
{
    PropertyValueEventArgs* eventArgs = nullptr;
    ErrCode err = 0;

    Property* prop = nullptr;
    String* name = nullptr;
    String_createString(&name, "test_property");
    Integer* defaultValue = nullptr;
    Integer_createInteger(&defaultValue, 10);
    Boolean* visible = nullptr;
    Boolean_createBoolean(&visible, True);

    err = Property_createIntProperty(&prop, name, defaultValue, visible);
    ASSERT_EQ(err, 0);

    Integer* value1 = nullptr;
    Integer* value2 = nullptr;

    Integer_createInteger(&value1, 20);
    Integer_createInteger(&value2, 30);

    err = PropertyValueEventArgs_createPropertyValueEventArgs(
        &eventArgs, prop, value2, value1, PropertyEventType::PropertyEventTypeEventTypeUpdate, False);
    ASSERT_EQ(err, 0);
    ASSERT_NE(eventArgs, nullptr);

    Integer* valueOut = nullptr;
    PropertyValueEventArgs_getValue(eventArgs, (BaseObject**) &valueOut);
    ASSERT_NE(valueOut, nullptr);
    Bool equal = False;
    err = BaseObject_equals(valueOut, value2, &equal);
    ASSERT_EQ(equal, True);
    BaseObject_releaseRef(valueOut);

    PropertyValueEventArgs_getOldValue(eventArgs, (BaseObject**) &valueOut);
    ASSERT_NE(valueOut, nullptr);
    err = BaseObject_equals(valueOut, value1, &equal);
    ASSERT_EQ(equal, True);

    BaseObject_releaseRef(valueOut);
    BaseObject_releaseRef(value1);
    BaseObject_releaseRef(value2);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(defaultValue);
    BaseObject_releaseRef(visible);
    BaseObject_releaseRef(prop);
    BaseObject_releaseRef(eventArgs);
}

TEST_F(CCoreobjectsTest, Unit)
{
    UnitBuilder* unitBuilder = nullptr;
    ErrCode err = 0;
    String* name = nullptr;
    String_createString(&name, "test_unit");
    String* symbol = nullptr;
    String_createString(&symbol, "tu");

    err = UnitBuilder_createUnitBuilder(&unitBuilder);
    ASSERT_EQ(err, 0);
    ASSERT_NE(unitBuilder, nullptr);

    UnitBuilder_setName(unitBuilder, name);
    UnitBuilder_setSymbol(unitBuilder, symbol);

    Unit* unit = nullptr;
    err = UnitBuilder_build(unitBuilder, &unit);
    ASSERT_EQ(err, 0);
    ASSERT_NE(unit, nullptr);

    String* nameOut = nullptr;
    String* symbolOut = nullptr;
    Unit_getName(unit, &nameOut);
    Unit_getSymbol(unit, &symbolOut);
    ConstCharPtr nameStr = nullptr;
    ConstCharPtr symbolStr = nullptr;
    String_getCharPtr(nameOut, &nameStr);
    String_getCharPtr(symbolOut, &symbolStr);
    ASSERT_STREQ(nameStr, "test_unit");
    ASSERT_STREQ(symbolStr, "tu");

    BaseObject_releaseRef(nameOut);
    BaseObject_releaseRef(symbolOut);
    BaseObject_releaseRef(name);
    BaseObject_releaseRef(symbol);
    BaseObject_releaseRef(unit);
    BaseObject_releaseRef(unitBuilder);
}

TEST_F(CCoreobjectsTest, User)
{
    String* username = nullptr;
    String_createString(&username, "test_user");
    String* passwordHash = nullptr;
    String_createString(&passwordHash, "test_hash");
    List* groups = nullptr;
    List_createList(&groups);
    User* user = nullptr;
    User_createUser(&user, username, passwordHash, groups);

    String* usernameOut = nullptr;
    User_getUsername(user, &usernameOut);

    ConstCharPtr str = nullptr;
    String_getCharPtr(usernameOut, &str);
    ASSERT_STREQ(str, "test_user");

    BaseObject_releaseRef(usernameOut);
    BaseObject_releaseRef(user);
    BaseObject_releaseRef(username);
    BaseObject_releaseRef(passwordHash);
    BaseObject_releaseRef(groups);
}

TEST_F(CCoreobjectsTest, Validator)
{
    Validator* validator = nullptr;
    ErrCode err = 0;
    String* evalStr = nullptr;
    String_createString(&evalStr, "value > 5");
    err = Validator_createValidator(&validator, evalStr);
    ASSERT_EQ(err, 0);

    Integer* value = nullptr;
    Integer_createInteger(&value, 10);
    err = Validator_validate(validator, nullptr, value);
    ASSERT_EQ(err, 0);
    Integer* invalidValue = nullptr;
    Integer_createInteger(&invalidValue, 3);
    err = Validator_validate(validator, nullptr, invalidValue);
    ASSERT_NE(err, 0);

    BaseObject_releaseRef(value);
    BaseObject_releaseRef(invalidValue);
    BaseObject_releaseRef(evalStr);
    BaseObject_releaseRef(validator);
}
