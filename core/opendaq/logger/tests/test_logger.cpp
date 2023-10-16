#include <gtest/gtest.h>
#include <testutils/testutils.h>
#include <coretypes/intfs.h>
#include <coretypes/impl.h>
#include "invalid_logger_sink.h"
#include <opendaq/logger_ptr.h>
#include <thread>
#include <opendaq/logger_factory.h>

using LoggerTest = testing::Test;

using namespace daq;

TEST_F(LoggerTest, Create)
{
    ASSERT_NO_THROW(Logger());
}

TEST_F(LoggerTest, CreateWithFileName)
{
    ASSERT_NO_THROW(Logger("LoggerFile.log", LogLevel::Trace));
}

TEST_F(LoggerTest, CreateWithSinks)
{
    ASSERT_NO_THROW(LoggerWithSinks(DefaultSinks(), LogLevel::Trace));
}

TEST_F(LoggerTest, CreateWithoutSinks)
{
    ASSERT_THROW(LoggerWithSinks({}), ArgumentNullException);
}

TEST_F(LoggerTest, CreateWithInvalidSink)
{
    LoggerSinkPtr sink;
    ErrCode err = createObject<ILoggerSink, InvalidLoggerSink>(&sink);
    ASSERT_TRUE(OPENDAQ_SUCCEEDED(err));

    ASSERT_THROW(LoggerWithSinks({sink}), InvalidTypeException);
}

TEST_F(LoggerTest, CreateWithSinkStdErr)
{
    auto sink = StdErrLoggerSink();

    ASSERT_NO_THROW(LoggerWithSinks({sink}));
}

TEST_F(LoggerTest, CreateWithSinkStdOut)
{
    auto sink = StdOutLoggerSink();

    ASSERT_NO_THROW(LoggerWithSinks({sink}));
}

TEST_F(LoggerTest, CreateWithSinkRotatingFile)
{
    auto sink = RotatingFileLoggerSink("test_log.log", 500, 1);

    ASSERT_NO_THROW(LoggerWithSinks({sink}));
}

TEST_F(LoggerTest, CreateWithMultipleSinks)
{
    auto sink = RotatingFileLoggerSink("test_log.log", 500, 1);

    ASSERT_NO_THROW(LoggerWithSinks({sink, StdOutLoggerSink(), StdErrLoggerSink()}));
}

TEST_F(LoggerTest, AddComponentNull)
{
    auto logger = Logger();
    ASSERT_EQ(logger->addComponent(nullptr, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_EQ(logger->addComponent(StringPtr("test"), nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_THROW(logger.addComponent(nullptr), ArgumentNullException);
}

TEST_F(LoggerTest, AddComponentEmptyName)
{
    auto logger = Logger();
    ASSERT_THROW(logger.addComponent(""), InvalidParameterException);
}

TEST_F(LoggerTest, AddComponentExistingName)
{
    auto logger = Logger();
    logger.addComponent("test");
    ASSERT_THROW(logger.addComponent("test"), AlreadyExistsException);
}

TEST_F(LoggerTest, GetComponentNull)
{
    auto logger = Logger();
    ASSERT_EQ(logger->getComponent(nullptr, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_EQ(logger->getComponent(StringPtr("test"), nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_THROW(logger.getComponent(nullptr), ArgumentNullException);
}

TEST_F(LoggerTest, GetUnexistingComponent)
{
    auto logger = Logger();
    LoggerComponentPtr loggerComponent;
    ASSERT_EQ(logger->getComponent(StringPtr("notexisted"), &loggerComponent), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(LoggerTest, AddAndGetComponentValid)
{
    auto logger = Logger();
    LoggerComponentPtr loggerComponent;
    ASSERT_NO_THROW(loggerComponent = logger.addComponent("test"));
    ASSERT_STREQ(loggerComponent.getName().getCharPtr(), "test");

    LoggerComponentPtr testComponent;
    ASSERT_NO_THROW(testComponent = logger.getComponent("test"));
    ASSERT_EQ(loggerComponent, testComponent);
}

TEST_F(LoggerTest, GetOrAddComponentNull)
{
    auto logger = Logger();
    ASSERT_EQ(logger->getOrAddComponent(nullptr, nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_EQ(logger->getOrAddComponent(StringPtr("test"), nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
    ASSERT_THROW(logger.getOrAddComponent(nullptr), ArgumentNullException);
}

TEST_F(LoggerTest, GetOrAddComponentEmptyName)
{
    auto logger = Logger();
    ASSERT_THROW(logger.getOrAddComponent(""), InvalidParameterException);
}

TEST_F(LoggerTest, GetOrAddComponentExistingName)
{
    auto logger = Logger();
    auto addedComponent = logger.addComponent("test");

    LoggerComponentPtr resultOfGetOrAddComponent;
    ASSERT_NO_THROW(resultOfGetOrAddComponent = logger.getOrAddComponent("test"));
    ASSERT_EQ(addedComponent, resultOfGetOrAddComponent);

    LoggerComponentPtr resultOfGetComponent;
    ASSERT_NO_THROW(resultOfGetComponent = logger.getComponent("test"));
    ASSERT_EQ(resultOfGetComponent, resultOfGetOrAddComponent);
    ASSERT_EQ(resultOfGetComponent, addedComponent);
}

TEST_F(LoggerTest, GetOrAddComponentUnexistingName)
{
    auto logger = Logger();
    LoggerComponentPtr resultOfGetOrAddComponent;
    ASSERT_NO_THROW(resultOfGetOrAddComponent = logger.getOrAddComponent("test"));
    ASSERT_STREQ(resultOfGetOrAddComponent.getName().getCharPtr(), "test");

    LoggerComponentPtr resultOfGetComponent;
    ASSERT_NO_THROW(resultOfGetComponent = logger.getComponent("test"));
    ASSERT_EQ(resultOfGetOrAddComponent, resultOfGetComponent);

    LoggerComponentPtr resultOfGetOrAddComponent2;
    ASSERT_NO_THROW(resultOfGetOrAddComponent2 = logger.getOrAddComponent("test"));
    ASSERT_EQ(resultOfGetOrAddComponent2, resultOfGetComponent);
}

TEST_F(LoggerTest, RemoveComponentNull)
{
    auto logger = Logger();
    ASSERT_EQ(logger->removeComponent(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(LoggerTest, RemoveUnexistingComponent)
{
    auto logger = Logger();
    ASSERT_EQ(logger->removeComponent(StringPtr("notexisted")), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(LoggerTest, RemoveComponentValid)
{
    auto logger = Logger();
    auto loggerComponent = logger.addComponent("test");
    ASSERT_STREQ(loggerComponent.getName().getCharPtr(), "test");
    ASSERT_NO_THROW(logger.removeComponent("test"));
    LoggerComponentPtr testComponent;
    ASSERT_EQ(logger->getComponent(StringPtr("test"), &testComponent), OPENDAQ_ERR_NOTFOUND);
}

TEST_F(LoggerTest, AddComponentAfterRemove)
{
    auto logger = Logger();
    auto loggerComponent = logger.addComponent("test");
    logger.removeComponent("test");
    ASSERT_NO_THROW(logger.addComponent("test"));
}

TEST_F(LoggerTest, GetOrAddComponentAfterRemove)
{
    auto logger = Logger();
    auto loggerComponent = logger.addComponent("test");
    logger.removeComponent("test");
    ASSERT_NO_THROW(logger.getOrAddComponent("test"));
}

TEST_F(LoggerTest, GetLevelNull)
{
    auto logger = Logger();

    ASSERT_EQ(logger->getLevel(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(LoggerTest, SetLevel)
{
    auto logger = Logger();

    logger.setLevel(LogLevel::Trace);
    ASSERT_EQ(logger.getLevel(), LogLevel::Trace);

    logger.setLevel(LogLevel::Debug);
    ASSERT_EQ(logger.getLevel(), LogLevel::Debug);

    logger.setLevel(LogLevel::Info);
    ASSERT_EQ(logger.getLevel(), LogLevel::Info);

    logger.setLevel(LogLevel::Warn);
    ASSERT_EQ(logger.getLevel(), LogLevel::Warn);

    logger.setLevel(LogLevel::Error);
    ASSERT_EQ(logger.getLevel(), LogLevel::Error);

    logger.setLevel(LogLevel::Critical);
    ASSERT_EQ(logger.getLevel(), LogLevel::Critical);

    logger.setLevel(LogLevel::Off);
    ASSERT_EQ(logger.getLevel(), LogLevel::Off);
}


TEST_F(LoggerTest, AddComponentInheritLevel)
{
    auto logger = Logger();

    logger.setLevel(LogLevel::Error);
    auto loggerComponent = logger.addComponent("test");
    ASSERT_EQ(logger.getComponent("test").getLevel(), LogLevel::Error);
}

TEST_F(LoggerTest, ComponentChangeLevel)
{
    auto logger = Logger();

    logger.setLevel(LogLevel::Error);
    auto loggerComponent = logger.addComponent("test");
    ASSERT_EQ(logger.getComponent("test").getLevel(), LogLevel::Error);

    logger.setLevel(LogLevel::Trace);
    ASSERT_EQ(logger.getComponent("test").getLevel(), LogLevel::Error);

    loggerComponent.setLevel(LogLevel::Warn);
    ASSERT_EQ(logger.getComponent("test").getLevel(), LogLevel::Warn);
}

TEST_F(LoggerTest, GetComponentsNull)
{
    auto logger = Logger();
    ASSERT_EQ(logger->getComponents(nullptr), OPENDAQ_ERR_ARGUMENT_NULL);
}

TEST_F(LoggerTest, Components)
{
    const auto logger = Logger();
    auto components = logger.getComponents();
    ASSERT_EQ(components.getCount(), 0u);

    auto component1 = logger.addComponent("test1");
    components = logger.getComponents();
    ASSERT_EQ(components.getCount(), 1u);
    ASSERT_EQ(components[0], component1);

    auto component2 = logger.addComponent("test2");
    components = logger.getComponents();
    ASSERT_EQ(components.getCount(), 2u);

    auto component3 = logger.getOrAddComponent("test1");
    components = logger.getComponents();
    ASSERT_EQ(components.getCount(), 2u);

    auto component4 = logger.getOrAddComponent("test2");
    components = logger.getComponents();
    ASSERT_EQ(components.getCount(), 2u);

    logger.removeComponent("test1");
    components = logger.getComponents();
    ASSERT_EQ(components.getCount(), 1u);
    ASSERT_EQ(components[0], component2);

    logger.removeComponent("test2");
    components = logger.getComponents();
    ASSERT_EQ(components.getCount(), 0u);
}
