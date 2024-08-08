#include "coreobjects/property_object_factory.h"
#include "opcuatms_client/objects/tms_client_folder_factory.h"
#include "opcuatms_server/objects/tms_server_folder.h"
#include "opendaq/context_factory.h"
#include "tms_object_integration_test.h"
#include "opcuatms_client/objects/tms_client_io_folder_factory.h"
#include "opendaq/io_folder_factory.h"
#include "opendaq/mock/mock_channel_factory.h"

using namespace daq;
using namespace opcua::tms;
using namespace opcua;

struct RegisteredFolder
{
    TmsServerFolderPtr serverObject;
    FolderPtr serverFolder;
    FolderPtr clientFolder;
};

class TmsFolderTest : public TmsObjectIntegrationTest
{
public:
    FolderPtr createTestFolder()
    {
        auto folder1 = Folder(NullContext(), nullptr, "parent");
        auto folder2 = Folder(NullContext(), folder1, "child");
        folder1.addItem(folder2);
        auto leafFolder = Folder(NullContext(), folder2, "folder");
        folder2.addItem(leafFolder);

        folder2.addProperty(StringProperty("foo", "bar"));
        auto obj = PropertyObject();
        obj.addProperty(IntProperty("Int", 0));
        leafFolder.addProperty(ObjectProperty("obj", obj));

        folder1.getTags().asPtr<ITagsPrivate>().add("tag1");
        folder2.getTags().asPtr<ITagsPrivate>().add("tag2");

        return folder1;
    }

    FolderPtr createTestIOFolder()
    {
        auto folder1 = IoFolder(NullContext(), nullptr, "parent");
        auto folder2 = IoFolder(NullContext(), folder1, "child");
        folder1.addItem(folder2);
        auto channel = MockChannel(NullContext(), folder2, "channel");
        folder2.addItem(channel);

        return folder1;
    }

    RegisteredFolder registerTestFolder(const FolderPtr& testFolder)
    {
        RegisteredFolder folder{};

        folder.serverFolder = testFolder;
        folder.serverObject = std::make_shared<TmsServerFolder>(folder.serverFolder, this->getServer(), ctx, serverContext);
        auto nodeId = folder.serverObject->registerOpcUaNode();
        if (testFolder.supportsInterface<IIoFolderConfig>())
            folder.clientFolder = TmsClientIoFolder(NullContext(), nullptr, "test", clientContext, nodeId);
        else
            folder.clientFolder = TmsClientFolder(NullContext(), nullptr, "test", clientContext, nodeId);

        return folder;
    }
};

TEST_F(TmsFolderTest, Create)
{
    auto folder = createTestFolder();
    auto serverFolder = TmsServerFolder(folder, this->getServer(), ctx, serverContext);
}

TEST_F(TmsFolderTest, Register)
{
    auto folder = registerTestFolder(createTestFolder());
}

TEST_F(TmsFolderTest, Active)
{
    auto folder = registerTestFolder(createTestFolder());

    folder.clientFolder.setActive(false);
    ASSERT_EQ(folder.serverFolder.getActive(), folder.clientFolder.getActive());

    folder.clientFolder.setActive(true);
    ASSERT_EQ(folder.serverFolder.getActive(), folder.clientFolder.getActive());

    folder.clientFolder.getItems()[0].setActive(false);
    ASSERT_EQ(folder.serverFolder.getItems()[0].getActive(), folder.clientFolder.getItems()[0].getActive());

    
    folder.clientFolder.getItems()[0].setActive(false);
    ASSERT_EQ(folder.serverFolder.getItems()[0].getActive(), folder.clientFolder.getItems()[0].getActive());

    
    folder.clientFolder.getItems()[0].asPtr<IFolder>().getItems()[0].setActive(false);
    ASSERT_EQ(folder.serverFolder.getItems()[0].asPtr<IFolder>().getItems()[0].getActive(),
              folder.clientFolder.getItems()[0].asPtr<IFolder>().getItems()[0].getActive());

    
    folder.clientFolder.getItems()[0].asPtr<IFolder>().getItems()[0].setActive(false);
    ASSERT_EQ(folder.serverFolder.getItems()[0].asPtr<IFolder>().getItems()[0].getActive(),
              folder.clientFolder.getItems()[0].asPtr<IFolder>().getItems()[0].getActive());
}

TEST_F(TmsFolderTest, Tags)
{
    auto folder = registerTestFolder(createTestFolder());

    auto serverTags = folder.serverFolder.getTags().getList();
    auto clientTags = folder.clientFolder.getTags().getList();
    ASSERT_EQ(serverTags.getCount(), clientTags.getCount());
    ASSERT_EQ(serverTags[0], clientTags[0]);

    auto serverTags1 = folder.serverFolder.getItems()[0].getTags().getList();
    auto clientTags2 = folder.clientFolder.getItems()[0].getTags().getList();
    ASSERT_EQ(serverTags1.getCount(), clientTags2.getCount());
    ASSERT_EQ(serverTags1[0], clientTags2[0]);
}

TEST_F(TmsFolderTest, Properties)
{
    auto folder = registerTestFolder(createTestFolder());

    PropertyObjectPtr serverObj = folder.serverFolder.getItems()[0].asPtr<IFolder>().getItems()[0].getPropertyValue("obj");
    PropertyObjectPtr clientObj = folder.clientFolder.getItems()[0].asPtr<IFolder>().getItems()[0].getPropertyValue("obj");
    ASSERT_EQ(serverObj.getPropertyValue("Int"), clientObj.getPropertyValue("Int"));

    ASSERT_EQ(folder.serverFolder.getItems()[0].getPropertyValue("foo"), folder.clientFolder.getItems()[0].getPropertyValue("foo"));

    folder.clientFolder.getItems()[0].setPropertyValue("foo", "notbar");
    ASSERT_EQ(folder.serverFolder.getItems()[0].getPropertyValue("foo"), folder.clientFolder.getItems()[0].getPropertyValue("foo"));
}

TEST_F(TmsFolderTest, IOFolder)
{
    auto folder = registerTestFolder(createTestIOFolder());

    ASSERT_TRUE(folder.serverFolder.asPtrOrNull<IIoFolderConfig>().assigned());
    ASSERT_TRUE(folder.clientFolder.asPtrOrNull<IIoFolderConfig>().assigned());

    ASSERT_TRUE(folder.serverFolder.getItems()[0].asPtrOrNull<IIoFolderConfig>().assigned());
    ASSERT_TRUE(folder.clientFolder.getItems()[0].asPtrOrNull<IIoFolderConfig>().assigned());
    
    ASSERT_TRUE(folder.serverFolder.getItems()[0].asPtrOrNull<IFolder>().getItems()[0].asPtrOrNull<IChannel>().assigned());
    ASSERT_TRUE(folder.clientFolder.getItems()[0].asPtrOrNull<IFolder>().getItems()[0].asPtrOrNull<IChannel>().assigned());
}

TEST_F(TmsFolderTest, IOFolderNodeOrder)
{
    auto folder = IoFolder(NullContext(), nullptr, "parent");
    for (int i = 0; i < 100; ++i)
    {
        folder.addItem(IoFolder(NullContext(), folder, "child" + std::to_string(i)));
    }

    for (int i = 0; i < 10; ++i)
    {
        folder.addItem(MockChannel(NullContext(), folder, "channel" + std::to_string(i)));
    }

    auto registered = registerTestFolder(folder);

    auto serverItems = folder.getItems();
    auto clientItems = registered.clientFolder.getItems();

    for (SizeT i = 0; i < serverItems.getCount(); ++i)
        ASSERT_EQ(serverItems[i].getName(), clientItems[i].getName());
}

TEST_F(TmsFolderTest, FolderNodeOrder)
{
    auto folder = Folder(NullContext(), nullptr, "parent");
    for (int i = 0; i < 100; ++i)
    {
        folder.addItem(Folder(NullContext(), folder, "child" + std::to_string(i)));
    }

    auto registered = registerTestFolder(folder);

    auto serverItems = folder.getItems();
    auto clientItems = registered.clientFolder.getItems();

    for (SizeT i = 0; i < serverItems.getCount(); ++i)
        ASSERT_EQ(serverItems[i].getName(), clientItems[i].getName());
}
