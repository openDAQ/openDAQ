/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <map>

#include "opcuashared/opcua.h"
#include "opcuashared/node/opcuanodeid.h"
#include "opcuaclient/opcuaconnectionbroker.h"
#include <opcuashared/opcuaobject.h>

BEGIN_NAMESPACE_OPCUA

struct OpcUaBrowseTransaction
{
    virtual UA_BrowseResult* getResults() = 0;
    virtual size_t getResultsSize() = 0;
    virtual const UA_ResponseHeader& getResponseHeader() = 0;
};

template <typename T>
struct OpcUaBrowseTransactionT : public OpcUaBrowseTransaction
{
    OpcUaObject<T> response;

    UA_BrowseResult* getResults() override
    {
        return response->results;
    };

    size_t getResultsSize() override
    {
        return response->resultsSize;
    };

    const UA_ResponseHeader& getResponseHeader() override
    {
        return response->responseHeader;
    };
};

using OpcUaBrowseTransactionPtr = std::shared_ptr<OpcUaBrowseTransaction>;
using OpcUaBrowseTransaction_First = OpcUaBrowseTransactionT<UA_BrowseResponse>;
using OpcUaBrowseTransaction_FirstPtr = std::shared_ptr<OpcUaBrowseTransaction_First>;
using OpcUaBrowseTransaction_Next = OpcUaBrowseTransactionT<UA_BrowseNextResponse>;
using OpcUaBrowseTransaction_NextPtr = std::shared_ptr<OpcUaBrowseTransaction_Next>;

class OpcUaBrowser
{
public:
    OpcUaBrowser(OpcUaObject<UA_BrowseRequest>& request, const OpcUaConnectionBrokerPtr& connection);
    std::vector<OpcUaBrowseTransactionPtr> browse();

private:
    OpcUaBrowseTransactionPtr browseTransaction();
    void dummy();
    void browseNextTransaction2();
    //OpcUaBrowseTransactionPtr browseNextTransaction1();
    //OpcUaBrowseTransactionPtr browseNextTransaction(UA_ByteString* contPoint);
    //std::unique_ptr<OpcUaObject<UA_BrowseNextRequest>> prepareNextRequest(UA_ByteString* contPoint);
    void validateTransactionStatus(const OpcUaBrowseTransactionPtr& transaction);
    void validateTransactionStatus(UA_StatusCode status);

    OpcUaObject<UA_BrowseRequest>& request;
    const OpcUaConnectionBrokerPtr& connection;
    std::vector<OpcUaBrowseTransactionPtr> transactions;
};

END_NAMESPACE_OPCUA
