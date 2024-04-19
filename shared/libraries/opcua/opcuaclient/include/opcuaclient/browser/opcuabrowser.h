/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

#include <tsl/ordered_map.h>

#include "opcuashared/opcua.h"
#include "opcuashared/opcuanodeid.h"
#include "opcuaclient/opcuaclient.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

struct OpcUaBrowseTransaction
{
    virtual UA_BrowseResult* getResults() = 0;
    virtual size_t getResultsSize() const = 0;
    virtual const UA_ResponseHeader& getResponseHeader() const = 0;
};

template <typename T>
struct OpcUaBrowseTransactionT : public OpcUaBrowseTransaction
{
    OpcUaObject<T> response;

    UA_BrowseResult* getResults() override
    {
        return response->results;            
    };

    size_t getResultsSize() const override
    {
        return response->resultsSize;
    };

    const UA_ResponseHeader& getResponseHeader() const override
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
    OpcUaBrowser(const OpcUaNodeId& nodeId, const OpcUaClientPtr& client);
    OpcUaBrowser(const OpcUaObject<UA_BrowseRequest>& request, const OpcUaClientPtr& client);
    std::vector<UA_ReferenceDescription>& browse();
    tsl::ordered_map<OpcUaNodeId, OpcUaObject<UA_ReferenceDescription>> referencesByNodeId();
    tsl::ordered_map<std::string, OpcUaObject<UA_ReferenceDescription>> referencesByBrowseName();

private:
    OpcUaBrowseTransactionPtr browseTransaction();
    OpcUaBrowseTransactionPtr browseNextTransaction(UA_ByteString* contPoint);
    OpcUaObject<UA_BrowseNextRequest> prepareNextRequest(UA_ByteString* contPoint);
    void validateTransactionStatus(const OpcUaBrowseTransactionPtr& transaction);

    OpcUaObject<UA_BrowseRequest> request;
    OpcUaClientPtr client;
    std::vector<OpcUaBrowseTransactionPtr> transactions;
    std::vector<UA_ReferenceDescription> references;

};


END_NAMESPACE_OPENDAQ_OPCUA
