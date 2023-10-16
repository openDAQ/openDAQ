#include "opcuaclient/opcuacallmethodrequest.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaCallMethodRequest::OpcUaCallMethodRequest(const OpcUaNodeId& methodId,
                                               const OpcUaNodeId& objectId,
                                               size_t inputArgumentsSize,
                                               UA_Variant* inputArguments)
    : OpcUaObject<UA_CallMethodRequest>()
{
    value.methodId = methodId.copyAndGetDetachedValue();
    value.objectId = objectId.copyAndGetDetachedValue();
    value.inputArgumentsSize = inputArgumentsSize;
    CheckStatusCodeException(
        UA_Array_copy(inputArguments, inputArgumentsSize, (void**) &value.inputArguments, &UA_TYPES[UA_TYPES_VARIANT]));
}

OpcUaCallMethodRequestWithCallback::OpcUaCallMethodRequestWithCallback(const OpcUaNodeId& methodId,
                                                                       const OpcUaNodeId& objectId,
                                                                       const ProcessFunctionType& processFunction,
                                                                       size_t inputArgumentsSize,
                                                                       UA_Variant* inputArguments)
    : OpcUaCallMethodRequest(methodId, objectId, inputArgumentsSize, inputArguments)
    , processFunction(processFunction)
{
}

END_NAMESPACE_OPENDAQ_OPCUA
