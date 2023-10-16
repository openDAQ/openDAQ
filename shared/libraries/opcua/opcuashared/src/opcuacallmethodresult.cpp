#include "opcuashared/opcuacallmethodresult.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaCallMethodResult::OpcUaCallMethodResult(const UA_CallMethodResult& callMethodResult)
    : callMethodResult(callMethodResult)
{
}

OpcUaCallMethodResult::~OpcUaCallMethodResult()
{
}

size_t OpcUaCallMethodResult::getOutputArgumentsSize() const
{
    return callMethodResult.outputArgumentsSize;
}

const UA_StatusCode& OpcUaCallMethodResult::getStatusCode() const
{
    return callMethodResult.statusCode;
}

bool OpcUaCallMethodResult::isStatusOK() const
{
    return (getStatusCode() == UA_STATUSCODE_GOOD);
}

OpcUaVariant OpcUaCallMethodResult::getOutputArgument(size_t i) const
{
    if (i < callMethodResult.outputArgumentsSize)
        return OpcUaVariant(callMethodResult.outputArguments[i], true);
    throw std::out_of_range("index of output argument is out of range");
}

const UA_CallMethodResult& OpcUaCallMethodResult::getCallMethodResult() const
{
    return callMethodResult;
}

OpcUaCallMethodResult::operator const UA_CallMethodResult&() const
{
    return getCallMethodResult();
}

END_NAMESPACE_OPENDAQ_OPCUA
