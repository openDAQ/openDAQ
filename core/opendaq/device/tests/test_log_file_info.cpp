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
    ASSERT_EQ(logInfoBuilder.getId(), nullptr);
    ASSERT_EQ(logInfoBuilder.getDescription(), nullptr);
    ASSERT_EQ(logInfoBuilder.getEncoding(), LogFileEncodingType::Unknown);
    ASSERT_EQ(logInfoBuilder.getSize(), 0);
    ASSERT_EQ(logInfoBuilder.getLastModified(), nullptr);
}

TEST_F(LogFileInfoTest, BuilderSetGet)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    logInfoBuilder.setName("log_file_name");
    logInfoBuilder.setLocalPath("log_file_path");
    logInfoBuilder.setId("log_file_id");
    logInfoBuilder.setDescription("log_file_description");
    logInfoBuilder.setEncoding(LogFileEncodingType::Utf8);
    logInfoBuilder.setSize(100);
    logInfoBuilder.setLastModified("2022-01-01T00:00:00Z");

    ASSERT_EQ(logInfoBuilder.getName(), "log_file_name");
    ASSERT_EQ(logInfoBuilder.getLocalPath(), "log_file_path");
    ASSERT_EQ(logInfoBuilder.getId(), "log_file_id");
    ASSERT_EQ(logInfoBuilder.getDescription(), "log_file_description");
    ASSERT_EQ(logInfoBuilder.getEncoding(), LogFileEncodingType::Utf8);
    ASSERT_EQ(logInfoBuilder.getSize(), 100);
    ASSERT_EQ(logInfoBuilder.getLastModified(), "2022-01-01T00:00:00Z");
}

TEST_F(LogFileInfoTest, CreateFromBuilder)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    logInfoBuilder.setName("log_file_name");
    logInfoBuilder.setLocalPath("log_file_path");
    logInfoBuilder.setDescription("log_file_description");
    logInfoBuilder.setEncoding(LogFileEncodingType::Utf8);
    logInfoBuilder.setLastModified("2022-01-01T00:00:00Z");

    auto logInfo = logInfoBuilder.build();
    ASSERT_TRUE(logInfo.assigned());
    ASSERT_EQ(logInfo.getName(), "log_file_name");
    ASSERT_EQ(logInfo.getLocalPath(), "log_file_path");
    ASSERT_EQ(logInfo.getId(), "log_file_path/log_file_name");
    ASSERT_EQ(logInfo.getDescription(), "log_file_description");
    ASSERT_EQ(logInfo.getEncoding(), LogFileEncodingType::Utf8);
    ASSERT_EQ(logInfo.getSize(), 0);
    ASSERT_EQ(logInfo.getLastModified(), "2022-01-01T00:00:00Z");
}

TEST_F(LogFileInfoTest, MissedName)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    // logInfoBuilder.setName("log_file_name");
    logInfoBuilder.setLocalPath("log_file_path");
    logInfoBuilder.setDescription("log_file_description");
    logInfoBuilder.setEncoding(LogFileEncodingType::Utf8);
    logInfoBuilder.setLastModified("2022-01-01T00:00:00Z");

    ASSERT_ANY_THROW(logInfoBuilder.build());
}

TEST_F(LogFileInfoTest, MissedLocalPath)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    logInfoBuilder.setName("log_file_name");
    // logInfoBuilder.setLocalPath("log_file_path");
    logInfoBuilder.setDescription("log_file_description");
    logInfoBuilder.setEncoding(LogFileEncodingType::Utf8);
    logInfoBuilder.setLastModified("2022-01-01T00:00:00Z");

    auto logInfo = logInfoBuilder.build();
    ASSERT_TRUE(logInfo.assigned());
    ASSERT_EQ(logInfo.getName(), "log_file_name");
    ASSERT_EQ(logInfo.getLocalPath(), nullptr);
    ASSERT_EQ(logInfo.getId(), "log_file_name");
    ASSERT_EQ(logInfo.getDescription(), "log_file_description");
    ASSERT_EQ(logInfo.getEncoding(), LogFileEncodingType::Utf8);
    ASSERT_EQ(logInfo.getSize(), 0);
}

TEST_F(LogFileInfoTest, MissedLastModified)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    logInfoBuilder.setName("log_file_name");
    logInfoBuilder.setLocalPath("log_file_path");
    logInfoBuilder.setDescription("log_file_description");
    logInfoBuilder.setEncoding(LogFileEncodingType::Utf8);

    ASSERT_ANY_THROW(logInfoBuilder.build());
}

TEST_F(LogFileInfoTest, CustomId)
{
    auto logInfoBuilder = LogFileInfoBuilder();
    logInfoBuilder.setName("log_file_name");
    logInfoBuilder.setId("log_file_id");
    // logInfoBuilder.setLocalPath("log_file_path");
    logInfoBuilder.setDescription("log_file_description");
    logInfoBuilder.setEncoding(LogFileEncodingType::Utf8);
    logInfoBuilder.setLastModified("2022-01-01T00:00:00Z");

    auto logInfo = logInfoBuilder.build();
    ASSERT_TRUE(logInfo.assigned());
    ASSERT_EQ(logInfo.getName(), "log_file_name");
    ASSERT_EQ(logInfo.getLocalPath(), nullptr);
    ASSERT_EQ(logInfo.getId(), "log_file_id");
    ASSERT_EQ(logInfo.getDescription(), "log_file_description");
    ASSERT_EQ(logInfo.getEncoding(), LogFileEncodingType::Utf8);
}

TEST(LogFileInfoTest, SerializeDeserializeEmptyFields)
{
    auto logInfo = LogFileInfoBuilder().setName("log_file_name")
                                       .setLastModified("2022-01-01T00:00:00Z")
                                       .build();

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
    ASSERT_EQ(logInfo.getSize(), newLogFileInfo.getSize());
    ASSERT_EQ(logInfo.getLastModified(), newLogFileInfo.getLastModified());

    serializer.reset();
    newLogFileInfo.serialize(serializer);
    const auto newSerializedLogFileInfo = serializer.getOutput();

    ASSERT_EQ(serializedLogFileInfo, newSerializedLogFileInfo);
}

TEST(LogFileInfoTest, SerializeDeserialize)
{
    auto logInfo = LogFileInfoBuilder().setName("log_file_name")
                                       .setLocalPath("log_file_path")
                                       .setId("log_file_id")
                                       .setDescription("log_file_description")
                                       .setEncoding(LogFileEncodingType::Utf8)
                                       .setSize(100)
                                       .setLastModified("2022-01-01T00:00:00Z")
                                       .build();

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
    ASSERT_EQ(logInfo.getSize(), newLogFileInfo.getSize());
    ASSERT_EQ(logInfo.getLastModified(), newLogFileInfo.getLastModified());

    serializer.reset();
    newLogFileInfo.serialize(serializer);
    const auto newSerializedLogFileInfo = serializer.getOutput();

    ASSERT_EQ(serializedLogFileInfo, newSerializedLogFileInfo);
}

END_NAMESPACE_OPENDAQ