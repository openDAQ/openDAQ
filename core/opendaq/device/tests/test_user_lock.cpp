#include <opendaq/user_lock_factory.h>
#include <gtest/gtest.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/authentication_provider_factory.h>
#include <opendaq/logger_factory.h>
#include <opendaq/scheduler_factory.h>
#include <opendaq/context_factory.h>
#include <opendaq/component_deserialize_context_factory.h>

using namespace daq;

using UserLockTest = testing::Test;

TEST_F(UserLockTest, Create)
{
    auto lock = UserLock();
    ASSERT_TRUE(lock.assigned());
    ASSERT_FALSE(lock.isLocked());
}

TEST_F(UserLockTest, LockUnlock)
{
    auto lock = UserLock();
    ASSERT_FALSE(lock.isLocked());

    lock.lock();
    ASSERT_TRUE(lock.isLocked());

    lock.unlock();
    ASSERT_FALSE(lock.isLocked());
}

TEST_F(UserLockTest, LockUnlockUser)
{
    auto jure = User("jure", "jure");
    auto tomaz = User("tomaz", "tomaz");

    auto lock = UserLock();
    ASSERT_FALSE(lock.isLocked());

    lock.lock(jure);
    ASSERT_TRUE(lock.isLocked());

    ASSERT_THROW(lock.unlock(), AccessDeniedException);
    ASSERT_THROW(lock.unlock(tomaz), AccessDeniedException);
    lock.unlock(jure);
    ASSERT_FALSE(lock.isLocked());
}

TEST_F(UserLockTest, LockAnonymous)
{
    auto jure = User("jure", "jure");
    auto tomaz = User("tomaz", "tomaz");
    auto anonymous = User("", "");

    auto lock = UserLock();
    ASSERT_FALSE(lock.isLocked());

    lock.lock(anonymous);
    ASSERT_TRUE(lock.isLocked());
    lock.unlock();
    ASSERT_FALSE(lock.isLocked());

    lock.lock(anonymous);
    ASSERT_TRUE(lock.isLocked());
    lock.unlock(jure);
    ASSERT_FALSE(lock.isLocked());

    lock.lock(anonymous);
    ASSERT_TRUE(lock.isLocked());
    lock.unlock(tomaz);
    ASSERT_FALSE(lock.isLocked());

    lock.lock(anonymous);
    ASSERT_TRUE(lock.isLocked());
    lock.unlock(anonymous);
    ASSERT_FALSE(lock.isLocked());
}

TEST_F(UserLockTest, LockTwice)
{
    auto jure = User("jure", "jure");
    auto tomaz = User("tomaz", "tomaz");

    auto lock = UserLock();
    ASSERT_FALSE(lock.isLocked());

    lock.lock(jure);
    ASSERT_TRUE(lock.isLocked());
    lock.lock(jure);
    ASSERT_TRUE(lock.isLocked());

    ASSERT_THROW(lock.lock(tomaz), DeviceLockedException);
    ASSERT_THROW(lock.lock(), DeviceLockedException);
}

TEST_F(UserLockTest, UnlockTwice)
{
    auto jure = User("jure", "jure");
    auto tomaz = User("tomaz", "tomaz");

    auto lock = UserLock();
    ASSERT_FALSE(lock.isLocked());

    lock.lock(jure);
    ASSERT_TRUE(lock.isLocked());

    lock.unlock(jure);
    ASSERT_FALSE(lock.isLocked());
    lock.unlock(jure);
    ASSERT_FALSE(lock.isLocked());
}

TEST_F(UserLockTest, ForceUnlock)
{
    auto jure = User("jure", "jure");

    auto lock = UserLock();
    lock.lock(jure);
    ASSERT_TRUE(lock.isLocked());

    ASSERT_THROW(lock.unlock(), AccessDeniedException);
    lock.forceUnlock();
    ASSERT_FALSE(lock.isLocked());

    lock.forceUnlock();
    ASSERT_FALSE(lock.isLocked());
}

TEST_F(UserLockTest, Serialize)
{
    auto jure = User("jure", "jure");
    auto users = List<IUser>(jure);
    auto authenticationProvider = StaticAuthenticationProvider(false, users);

    auto logger = Logger();
    auto context = Context(Scheduler(logger, 1), logger, TypeManager(), nullptr, authenticationProvider);
    auto deserializeContext = ComponentDeserializeContext(context, nullptr, nullptr, "dev");

    auto lock = UserLock();
    lock.lock(jure);

    auto serializer = JsonSerializer();
    lock.serialize(serializer);
    const auto json = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    UserLockPtr lockNew = deserializer.deserialize(json, deserializeContext);

    ASSERT_TRUE(lockNew.assigned());
    ASSERT_TRUE(lockNew.isLocked());
    ASSERT_THROW(lockNew.unlock(), AccessDeniedException);
    lockNew.unlock(jure);
    ASSERT_FALSE(lockNew.isLocked());
}

TEST_F(UserLockTest, SerializeUnlocked)
{
    auto lock = UserLock();

    auto serializer = JsonSerializer();
    lock.serialize(serializer);
    const auto json = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    UserLockPtr lockNew = deserializer.deserialize(json);

    ASSERT_TRUE(lockNew.assigned());
    ASSERT_FALSE(lockNew.isLocked());
}

TEST_F(UserLockTest, SerializeNoContext)
{
    auto jure = User("jure", "jure");
    auto tomaz = User("tomaz", "tomaz");
    auto users = List<IUser>(jure, tomaz);

    auto authenticationProvider = StaticAuthenticationProvider(false, users);

    auto lock = UserLock();
    lock.lock(jure);

    auto serializer = JsonSerializer();
    lock.serialize(serializer);
    const auto json = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    UserLockPtr lockNew = deserializer.deserialize(json);

    ASSERT_TRUE(lockNew.assigned());
    ASSERT_TRUE(lockNew.isLocked());
    lockNew.unlock();
    ASSERT_FALSE(lockNew.isLocked());
}

TEST_F(UserLockTest, SerializeNoUser)
{
    auto jure = User("jure", "jure");
    auto tomaz = User("tomaz", "tomaz");
    auto users = List<IUser>(jure);

    auto authenticationProvider = StaticAuthenticationProvider(false, users);

    auto lock = UserLock();
    lock.lock(tomaz);

    auto serializer = JsonSerializer();
    lock.serialize(serializer);
    const auto json = serializer.getOutput();

    auto deserializer = JsonDeserializer();
    UserLockPtr lockNew = deserializer.deserialize(json, authenticationProvider);

    ASSERT_TRUE(lockNew.assigned());
    ASSERT_TRUE(lockNew.isLocked());
    lockNew.unlock();
    ASSERT_FALSE(lockNew.isLocked());
}

