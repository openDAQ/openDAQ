#include "opcuaservertesthelper.h"
#include <open62541/server_config_default.h>
#include <open62541/server.h>
#include <opcuashared/opcuanodeid.h>
#include <cstring>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaServerTestHelper::OpcUaServerTestHelper()
    : sessionTimeoutMs(-1)
{
}

OpcUaServerTestHelper::~OpcUaServerTestHelper()
{
    stop();
}

void OpcUaServerTestHelper::setSessionTimeout(double sessionTimeoutMs)
{
    this->sessionTimeoutMs = sessionTimeoutMs;
}

void OpcUaServerTestHelper::runServer()
{
    while (serverRunning)
        UA_Server_run_iterate(server, true);

    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
}

void OpcUaServerTestHelper::onConfigure(const OnConfigureCallback& callback)
{
    onConfigureCallback = callback;
}

void OpcUaServerTestHelper::startServer()
{
    serverRunning = true;

    UA_ServerConfig initConfig;
    std::memset(&initConfig, 0, sizeof(UA_ServerConfig));

    if (onConfigureCallback)
        onConfigureCallback(&initConfig);

    UA_ServerConfig_setMinimal(&initConfig, port, nullptr);
    server = UA_Server_newWithConfig(&initConfig);
    UA_ServerConfig* config = UA_Server_getConfig(server);

    if (sessionTimeoutMs > 0)
        config->maxSessionTimeout = sessionTimeoutMs;

#ifdef UA_ENABLE_WEBSOCKET_SERVER
    UA_ServerConfig_addNetworkLayerWS(config, 80, 0, 0);
#endif  // UA_ENABLE_WEBSOCKET_SERVER

    createModel();

    UA_Server_run_startup(server);

#if SYNTH_SERVER_DEBUG
    runServer();
#else
    serverThreadPtr = std::make_unique<std::thread>(&OpcUaServerTestHelper::runServer, this);
#endif
}

void OpcUaServerTestHelper::stop()
{
    serverRunning = false;
    if (serverThreadPtr)
    {
        serverThreadPtr->join();
        serverThreadPtr.reset();
    }
}

std::string OpcUaServerTestHelper::getServerUrl() const
{
    std::stringstream ss;
    ss << "opc.tcp://127.0.0.1";
    ss << ":";
    ss << port;
    return ss.str();
}

void OpcUaServerTestHelper::createModel()
{
    auto uaObjectsFolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);

    publishFolder("f1", &uaObjectsFolder);

    UA_Int32 myInt32;

    myInt32 = 33;
    publishVariable("f1.i", &myInt32, &UA_TYPES[UA_TYPES_INT32], OpcUaNodeId(1, "f1").getPtr());

    myInt32 = 41;
    publishVariable(".i32", &myInt32, &UA_TYPES[UA_TYPES_INT32], &uaObjectsFolder);

    UA_UInt32 myUInt32 = 19;
    publishVariable(".ui32", &myUInt32, &UA_TYPES[UA_TYPES_UINT32], &uaObjectsFolder);

    UA_Int16 myInt16 = 16;
    publishVariable(".i16", &myInt16, &UA_TYPES[UA_TYPES_INT16], &uaObjectsFolder);

    UA_UInt16 myUInt16 = 33;
    publishVariable(".ui16", &myUInt16, &UA_TYPES[UA_TYPES_UINT16], &uaObjectsFolder);

    UA_Int64 myInt64 = 64;
    publishVariable(".i64", &myInt64, &UA_TYPES[UA_TYPES_INT64], &uaObjectsFolder);

    UA_Boolean myBool = true;
    publishVariable(".b", &myBool, &UA_TYPES[UA_TYPES_BOOLEAN], &uaObjectsFolder);

    UA_Double myDouble = (UA_Double) 1885 / (UA_Double) 14442;
    publishVariable(".d", &myDouble, &UA_TYPES[UA_TYPES_DOUBLE], &uaObjectsFolder);

    UA_Float myFloat = (UA_Float) 1 / (UA_Float) 3;
    publishVariable(".f", &myFloat, &UA_TYPES[UA_TYPES_FLOAT], &uaObjectsFolder);

    UA_String myString = UA_STRING_ALLOC("Hello Dewesoft");
    publishVariable(".s", &myString, &UA_TYPES[UA_TYPES_STRING], &uaObjectsFolder);
    UA_String_clear(&myString);

    UA_Guid myGuid = UA_GUID("8a336ac1-8632-482c-a565-23e6a9ad1abc");
    publishVariable(".g", &myGuid, &UA_TYPES[UA_TYPES_GUID], &uaObjectsFolder);

    UA_StatusCode myStatus = UA_STATUSCODE_GOODSUBSCRIPTIONTRANSFERRED;
    publishVariable(".sc", &myStatus, &UA_TYPES[UA_TYPES_STATUSCODE], &uaObjectsFolder);

    // vectors

    UA_Int32 myVecInt32[] = {12, 13, 15, 18};
    publishVariable(".i32v", &myVecInt32, &UA_TYPES[UA_TYPES_INT32], &uaObjectsFolder, "en_US", 1, 4);

    UA_Int16 myVecInt16[] = {65, 18, 12, 17, 33, 10023, 12, 1, 0, -1};
    publishVariable(".i16v", &myVecInt16, &UA_TYPES[UA_TYPES_INT16], &uaObjectsFolder, "en_US", 1, 10);

    UA_Int64 myVecInt64[] = {55, 1993};
    publishVariable(".i64v", &myVecInt64, &UA_TYPES[UA_TYPES_INT64], &uaObjectsFolder, "en_US", 1, 2);

    UA_Boolean myVecBool[] = {true, false};
    publishVariable(".bv", &myVecBool, &UA_TYPES[UA_TYPES_BOOLEAN], &uaObjectsFolder, "en_US", 1, 2);

    UA_Double myVecDouble[] = {(UA_Double) 1993 / (UA_Double) 6625, (UA_Double) 185 / (UA_Double) 1443, (UA_Double) 1.44, (UA_Double) 9948};
    publishVariable(".dv", &myVecDouble, &UA_TYPES[UA_TYPES_DOUBLE], &uaObjectsFolder, "en_US", 1, 4);

    UA_Float myVecFloat[] = {(UA_Float) 7 / (UA_Float) 2, (UA_Float) 1 / (UA_Float) 5};
    publishVariable(".fv", &myVecFloat, &UA_TYPES[UA_TYPES_FLOAT], &uaObjectsFolder, "en_US", 1, 2);

    // methods

    publishMethod("hello.dewesoft", &uaObjectsFolder);

    // structures

    OpcUaNodeId structureNodeId(1, ".sctA");

    myInt32 = 56;
    publishVariable(".sctA", &myInt32, &UA_TYPES[UA_TYPES_INT32], &uaObjectsFolder);

    myInt32 = 5641;
    publishVariable(".sctA.i32", &myInt32, &UA_TYPES[UA_TYPES_INT32], structureNodeId.getPtr());

    UA_Double mySctDouble = (UA_Double) 9844 / (UA_Double) 19774;
    publishVariable(".sctA.d", &mySctDouble, &UA_TYPES[UA_TYPES_DOUBLE], structureNodeId.getPtr());

    UA_String mySctString = UA_STRING_ALLOC("Hello Dewesoft @ struct");
    publishVariable(".sctA.s", &mySctString, &UA_TYPES[UA_TYPES_STRING], structureNodeId.getPtr());
    UA_String_clear(&mySctString);
}

void OpcUaServerTestHelper::publishVariable(std::string identifier,
                                            const void* value,
                                            const UA_DataType* type,
                                            UA_NodeId* parentNodeId,
                                            const char* locale,
                                            uint16_t nodeIndex,
                                            size_t dimension)
{
    OpcUaObject<UA_VariableAttributes> attr = UA_VariableAttributes_default;
    attr->description = UA_LOCALIZEDTEXT_ALLOC(locale, identifier.c_str());
    attr->displayName = UA_LOCALIZEDTEXT_ALLOC(locale, identifier.c_str());
    attr->dataType = type->typeId;
    attr->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

    OpcUaNodeId newNodeId(nodeIndex, identifier);

    OpcUaObject<UA_QualifiedName> qualifiedName = UA_QUALIFIEDNAME_ALLOC(UA_UInt16(nodeIndex), identifier.c_str());

    if (dimension > 1)
    {
        attr->valueRank = 1;
        attr->arrayDimensionsSize = 1;
        attr->arrayDimensions = static_cast<UA_UInt32*>(UA_Array_new(1, &UA_TYPES[UA_TYPES_UINT32]));
        attr->arrayDimensions[0] = UA_UInt32(dimension);
        UA_Variant_setArrayCopy(&attr->value, value, dimension, type);
    }
    else
    {
        UA_Variant_setScalarCopy(&attr->value, value, type);
    }
    auto status = UA_Server_addVariableNode(server,
                              *newNodeId,
                              *parentNodeId,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                              *qualifiedName,
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                              *attr,
                              NULL,
                              NULL);

    CheckStatusCodeException(status);
}

void OpcUaServerTestHelper::publishFolder(const char* identifier, UA_NodeId* parentNodeId, const char* locale, int nodeIndex)
{
    OpcUaObject<UA_ObjectAttributes> attr = UA_ObjectAttributes_default;
    attr->description = UA_LOCALIZEDTEXT_ALLOC(locale, identifier);
    attr->displayName = UA_LOCALIZEDTEXT_ALLOC(locale, identifier);

    OpcUaNodeId newNodeId(nodeIndex, identifier);

    OpcUaObject<UA_QualifiedName> qualifiedName = UA_QUALIFIEDNAME_ALLOC(UA_UInt16(nodeIndex), identifier);

    auto status = UA_Server_addObjectNode(server,
                            *newNodeId,
                            *parentNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            *qualifiedName,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                            *attr,
                            NULL,
                            NULL);

    CheckStatusCodeException(status);
}

void OpcUaServerTestHelper::publishMethod(std::string identifier, UA_NodeId* parentNodeId, const char* locale, int nodeIndex)
{
    OpcUaObject<UA_Argument> inputArgument;
    inputArgument->description = UA_LOCALIZEDTEXT_ALLOC("en-US", "Input");
    inputArgument->name = UA_STRING_ALLOC("Input");
    inputArgument->dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    inputArgument->valueRank = -1;

    OpcUaObject<UA_Argument> outputArgument;
    outputArgument->description = UA_LOCALIZEDTEXT_ALLOC("en-US", "Output");
    outputArgument->name = UA_STRING_ALLOC("Output");
    outputArgument->dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    outputArgument->valueRank = -1;

    OpcUaObject<UA_MethodAttributes> attr = UA_MethodAttributes_default;
    attr->description = UA_LOCALIZEDTEXT_ALLOC(locale, identifier.c_str());
    attr->displayName = UA_LOCALIZEDTEXT_ALLOC(locale, identifier.c_str());
    attr->executable = true;
    attr->userExecutable = true;

    OpcUaNodeId newNodeId(nodeIndex, identifier);

    UA_Server_addMethodNode(server,
                            *newNodeId,
                            *parentNodeId,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
                            UA_QUALIFIEDNAME(1, (char*) identifier.c_str()),
                            *attr,
                            helloMethodCallback,
                            1,
                            inputArgument.get(),
                            1,
                            outputArgument.get(),
                            NULL,
                            NULL);
}

UA_StatusCode OpcUaServerTestHelper::helloMethodCallback(UA_Server* server,
                                                         const UA_NodeId* sessionId,
                                                         void* sessionHandle,
                                                         const UA_NodeId* methodId,
                                                         void* methodContext,
                                                         const UA_NodeId* objectId,
                                                         void* objectContext,
                                                         size_t inputSize,
                                                         const UA_Variant* input,
                                                         size_t outputSize,
                                                         UA_Variant* output)
{
    UA_String* inputStr = (UA_String*) input->data;
    std::string out = "Hello!";

    if (inputStr->length > 0)
    {
        std::string in = utils::ToStdString(*inputStr);
        std::stringstream ss;
        ss << out << " (R:" << in << ")";
        out = ss.str();
    }

    UA_String tmp = UA_STRING_ALLOC(out.c_str());
    UA_Variant_setScalarCopy(output, &tmp, &UA_TYPES[UA_TYPES_STRING]);
    UA_String_clear(&tmp);
    return UA_STATUSCODE_GOOD;
}

/*SampleServerTest*/

void BaseClientTest::SetUp()
{
    testing::Test::SetUp();
    testHelper.startServer();
}
void BaseClientTest::TearDown()
{
    testHelper.stop();
    testing::Test::TearDown();
}

std::string BaseClientTest::getServerUrl() const
{
    return testHelper.getServerUrl();
}

OpcUaClientPtr BaseClientTest::prepareAndConnectClient(int timeout)
{
    std::shared_ptr<OpcUaClient> client = std::make_shared<OpcUaClient>(getServerUrl());

    if (timeout >= 0)
        client->setTimeout(timeout);

    client->connect();
    return client;
}

END_NAMESPACE_OPENDAQ_OPCUA
