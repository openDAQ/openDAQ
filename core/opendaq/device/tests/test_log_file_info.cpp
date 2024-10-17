#include <gtest/gtest.h>
#include <opendaq/log_file_info_factory.h>
#include <fstream>
#include <chrono>
#include <coretypes/filesystem.h>
#include <coretypes/json_serializer_factory.h>

using LogFileInfoTest = testing::Test;

BEGIN_NAMESPACE_OPENDAQ

TEST_F(LogFileInfoTest, BuilderDefaultValues)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    ASSERT_EQ(logInfoBuilder.getName(), nullptr);
    ASSERT_EQ(logInfoBuilder.getLocalPath(), nullptr);
    ASSERT_EQ(logInfoBuilder.getDescription(), nullptr);
    ASSERT_EQ(logInfoBuilder.getEncoding(), LogFileEncodingType::Unknown);
}

TEST_F(LogFileInfoTest, BuilderSetGet)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    logInfoBuilder.setName("log_file_name");
    logInfoBuilder.setLocalPath("log_file_path");
    logInfoBuilder.setDescription("log_file_description");
    logInfoBuilder.setEncoding(LogFileEncodingType::Utf8);

    ASSERT_EQ(logInfoBuilder.getName(), "log_file_name");
    ASSERT_EQ(logInfoBuilder.getLocalPath(), "log_file_path");
    ASSERT_EQ(logInfoBuilder.getDescription(), "log_file_description");
    ASSERT_EQ(logInfoBuilder.getEncoding(), LogFileEncodingType::Utf8);
}

TEST_F(LogFileInfoTest, CreateFromBuilder)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    logInfoBuilder.setName("log_file_name");
    logInfoBuilder.setLocalPath("log_file_path");
    logInfoBuilder.setDescription("log_file_description");
    logInfoBuilder.setEncoding(LogFileEncodingType::Utf8);

    auto logInfo = logInfoBuilder.build();
    ASSERT_TRUE(logInfo.assigned());
    ASSERT_EQ(logInfo.getName(), "log_file_name");
    ASSERT_EQ(logInfo.getLocalPath(), "log_file_path");
    ASSERT_EQ(logInfo.getId(), "log_file_path/log_file_name");
    ASSERT_EQ(logInfo.getDescription(), "log_file_description");
    ASSERT_EQ(logInfo.getEncoding(), LogFileEncodingType::Utf8);
}

TEST_F(LogFileInfoTest, MissedName)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    // logInfoBuilder.setName("log_file_name");
    logInfoBuilder.setLocalPath("log_file_path");
    logInfoBuilder.setDescription("log_file_description");
    logInfoBuilder.setEncoding(LogFileEncodingType::Utf8);

    ASSERT_ANY_THROW(logInfoBuilder.build());
}

TEST_F(LogFileInfoTest, MissedLocalPath)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    logInfoBuilder.setName("log_file_name");
    // logInfoBuilder.setLocalPath("log_file_path");
    logInfoBuilder.setDescription("log_file_description");
    logInfoBuilder.setEncoding(LogFileEncodingType::Utf8);

    auto logInfo = logInfoBuilder.build();
    ASSERT_TRUE(logInfo.assigned());
    ASSERT_EQ(logInfo.getName(), "log_file_name");
    ASSERT_EQ(logInfo.getLocalPath(), nullptr);
    ASSERT_EQ(logInfo.getId(), "log_file_name");
    ASSERT_EQ(logInfo.getDescription(), "log_file_description");
    ASSERT_EQ(logInfo.getEncoding(), LogFileEncodingType::Utf8);
}

TEST_F(LogFileInfoTest, ObjectFromConstructor)
{
    auto logInfo = LogFileInfo("log_file_name", "log_file_path", "log_file_description", LogFileEncodingType::Utf8);
    ASSERT_TRUE(logInfo.assigned());
    ASSERT_EQ(logInfo.getName(), "log_file_name");
    ASSERT_EQ(logInfo.getLocalPath(), "log_file_path");
    ASSERT_EQ(logInfo.getId(), "log_file_path/log_file_name");
    ASSERT_EQ(logInfo.getDescription(), "log_file_description");
    ASSERT_EQ(logInfo.getEncoding(), LogFileEncodingType::Utf8);
}

TEST_F(LogFileInfoTest, ObjectFromConstructorMissedName)
{
    ASSERT_ANY_THROW(LogFileInfo(nullptr, "log_file_path", "log_file_description", LogFileEncodingType::Utf8));
}

TEST(LogFileInfoTest, GetSizeOfNotExistingFile)
{
    auto logInfo = LogFileInfo("not_existing_file");
    ASSERT_EQ(logInfo.getEncoding(), LogFileEncodingType::Unknown);
    ASSERT_ANY_THROW(logInfo.getSize());
}

void createFile(const std::string& path, const std::string& content) 
{
    // Open the file in binary mode
    std::ofstream outFile(path, std::ios::binary);

    if (!outFile) 
    {
        std::cerr << "Error opening file for writing: " << path << std::endl;
        return;
    }

    // Write content
    outFile.write(content.c_str(), content.size());
    outFile.close();
}


TEST(LogFileInfoTest, GetSizeOfExistingFile)
{
    const std::string path = "test_file";
    const std::string content = "test_content";
    createFile(path, content);

    auto logInfo = LogFileInfo(path);
    ASSERT_EQ(logInfo.getEncoding(), LogFileEncodingType::Unknown);
    ASSERT_EQ(logInfo.getSize(), content.size());
}

TEST(LogFileInfoTest, GetSizeOfEmptyFile)
{
    const std::string path = "empty_file";
    createFile(path, "");

    auto logInfo = LogFileInfo(path);
    ASSERT_EQ(logInfo.getEncoding(), LogFileEncodingType::Unknown);
    ASSERT_EQ(logInfo.getSize(), 0);
}

TEST(LogFileInfoTest, GetSizeWhileOpened)
{
    const std::string path = "opened_file";
    const std::string content = "test_content";

    std::ofstream outFile(path, std::ios::binary);

    if (!outFile) 
    {
        std::cerr << "Error opening file for writing: " << path << std::endl;
        return;
    }

    outFile.write(content.c_str(), content.size());
    outFile.flush();

    auto logInfo = LogFileInfo(path);
    ASSERT_EQ(logInfo.getEncoding(), LogFileEncodingType::Unknown);
    ASSERT_EQ(logInfo.getSize(), content.size());
}

std::string getFileLastModifiedTime(const std::string& path)
{
    auto ftime = fs::last_write_time(path);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
    );
    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&cftime), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

TEST(LogFileInfoTest, GetFileModified)
{
    const std::string path = "opened_file";
    const std::string content = "test_content";
    createFile(path, content);

    auto logInfo = LogFileInfo(path);
    std::string expectedLastModified = getFileLastModifiedTime(path);
    ASSERT_EQ(logInfo.getLastModified(), expectedLastModified);
}

TEST(LogFileInfoTest, GetNotExistingFileModified)
{
    const std::string path = "not_existing_file";
    auto logInfo = LogFileInfo(path);
    ASSERT_ANY_THROW(logInfo.getLastModified());
}

TEST(LogFileInfoTest, GetFileModifiedWhileOpened)
{
    const std::string path = "opened_file";
    const std::string content = "test_content";

    std::ofstream outFile(path, std::ios::binary);

    if (!outFile) 
    {
        std::cerr << "Error opening file for writing: " << path << std::endl;
        return;
    }

    std::string expectedLastModified = getFileLastModifiedTime(path);
    outFile.write(content.c_str(), content.size());
    outFile.flush();

    auto logInfo = LogFileInfo(path);

    ASSERT_EQ(logInfo.getLastModified(), expectedLastModified);
}

TEST(LogFileInfoTest, SerializeDeserialize)
{
    StringPtr name = "test_name";
    StringPtr localPath = "test_local_path";
    StringPtr description = "test_description";
    LogFileEncodingType encoding = LogFileEncodingType::Utf8;

    auto logInfo = LogFileInfo(localPath, name, description, encoding);

    const auto serializer = JsonSerializer();
    logInfo.serialize(serializer);
    const auto serializedLogFileInfo = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const LogFileInfoPtr newLogFileInfo = deserializer.deserialize(serializedLogFileInfo, nullptr, nullptr);

    ASSERT_EQ(logInfo.getName(), newLogFileInfo.getName());
    ASSERT_EQ(logInfo.getLocalPath(), newLogFileInfo.getLocalPath());
    ASSERT_EQ(logInfo.getId(), newLogFileInfo.getId());
    ASSERT_EQ(logInfo.getDescription(), newLogFileInfo.getDescription());
    ASSERT_EQ(logInfo.getEncoding(), newLogFileInfo.getEncoding());

    serializer.reset();
    newLogFileInfo.serialize(serializer);
    const auto newSerializedLogFileInfo = serializer.getOutput();

    ASSERT_EQ(serializedLogFileInfo, newSerializedLogFileInfo);
}

TEST(LogFileInfoTest, SerializeDeserializeEmptyFields)
{
    StringPtr name = "test_name";
    auto logInfo = LogFileInfo(name);

    const auto serializer = JsonSerializer();
    logInfo.serialize(serializer);
    const auto serializedLogFileInfo = serializer.getOutput();

    const auto deserializer = JsonDeserializer();

    const LogFileInfoPtr newLogFileInfo = deserializer.deserialize(serializedLogFileInfo, nullptr, nullptr);

    ASSERT_EQ(logInfo.getName(), newLogFileInfo.getName());
    ASSERT_EQ(logInfo.getLocalPath(), newLogFileInfo.getLocalPath());
    ASSERT_EQ(logInfo.getId(), newLogFileInfo.getId());
    ASSERT_EQ(logInfo.getDescription(), newLogFileInfo.getDescription());
    ASSERT_EQ(logInfo.getEncoding(), newLogFileInfo.getEncoding());

    serializer.reset();
    newLogFileInfo.serialize(serializer);
    const auto newSerializedLogFileInfo = serializer.getOutput();

    ASSERT_EQ(serializedLogFileInfo, newSerializedLogFileInfo);
}

END_NAMESPACE_OPENDAQ