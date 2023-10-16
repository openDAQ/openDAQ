#include <testutils/testutils.h>

#include "opcuashared/opcua.h"
#include "opcuashared/opcuacommon.h"
#include "opcuaservertesthelper.h"
#include <opcuaclient/opcuanodefactory.h>
#include <opcuaclient/browser/opcuanodefactorybrowser.h>
#include <opcuashared/node/opcuanodevariable.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using OpcUaNodeFactoryBrowserTest = BaseClientTest;

TEST_F_OPTIONAL(OpcUaNodeFactoryBrowserTest, Create)
{
    auto client = prepareAndConnectClient();
    auto nodeFactory = std::make_shared<OpcUaNodeFactory>(client);

    ASSERT_NO_THROW(OpcUaNodeFactoryBrowser(nodeFactory, client));
}

TEST_F_OPTIONAL(OpcUaNodeFactoryBrowserTest, Browse)
{
    auto client = prepareAndConnectClient();
    auto nodeFactory = std::make_shared<OpcUaNodeFactory>(client);
    OpcUaNodeFactoryBrowser browser(nodeFactory, client);
    browser.browseTree(OpcUaNodeId(1, "f1"));

    ASSERT_EQ(browser.getNodes().size(), 1u);

    auto node = browser.getNodes()[0];
    ASSERT_EQ(node->getBrowseName(), "f1.i");
    ASSERT_EQ(node->getNodeClass(), OpcUaNodeClass::Variable);

    auto nodeVariable = std::dynamic_pointer_cast<OpcUaNodeVariable>(node);
    ASSERT_NE(nodeVariable, nullptr);
    ASSERT_EQ(nodeVariable->getDataTypeNodeId(), UA_TYPES[UA_TYPES_INT32].typeId);
}

TEST_F_OPTIONAL(OpcUaNodeFactoryBrowserTest, BrowseDataTypes)
{
    auto client = prepareAndConnectClient();
    auto nodeFactory = std::make_shared<OpcUaNodeFactory>(client);
    OpcUaNodeFactoryBrowser browser(nodeFactory, client);
    browser.browseTree(OpcUaNodeId(UA_NS0ID_BASEDATATYPE));

    ASSERT_GE(browser.getNodes().size(), 16u);

    auto nodes = browser.getNodes();
    auto it = std::find_if(nodes.cbegin(), nodes.cend(), [](const OpcUaNodePtr& node) {return node->getBrowseName() == "Boolean"; });
    ASSERT_NE(it, nodes.cend());

    const auto& booleanNode = *it;

    ASSERT_EQ(booleanNode->getBrowseName(), "Boolean");
    ASSERT_EQ(booleanNode->getDisplayName(), "Boolean");
    ASSERT_EQ(booleanNode->getNodeId(), OpcUaNodeId(0, 1));
}

END_NAMESPACE_OPENDAQ_OPCUA
