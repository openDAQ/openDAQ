#include <gtest/gtest.h>
#include <coretypes/object_pool.h>
#include <stack>

using namespace daq::object_pool;

static unsigned int g_seed = 656121;

inline int fast_rand(void)
{
    g_seed = (214013 * g_seed + 2531011);
    return (g_seed >> 16) & 0x7FFF;
}

class StandardObject
{
public:
    StandardObject(int value)
        : value(value)
        , refCount(0)
    {
    }

    int getValue() const
    {
        return value;
    }

    size_t addRef()
    {
        return ++refCount;
    }

    size_t releaseRef()
    {
        assert(refCount > 0);
        if (--refCount == 0)
        {
            delete this;
            return 0;
        }
        return refCount;
    }

private:
    int value;
    size_t refCount;
};

class PoolObject
{
public:
    PoolObject* next;

    PoolObject(ObjectPool<PoolObject>* pool)
        : next(nullptr)
        , value(0)
        , pool(pool)
        , refCount(0)
    {
    }

    void reset(int value)
    {
        this->value = value;
    }

    int getValue() const
    {
        return value;
    }

    size_t addRef()
    {
        return ++refCount;
    }

    size_t releaseRef()
    {
        assert(refCount > 0);
        if (--refCount == 0)
        {
            pool->addToFreeList(this);
            return 0;
        }
        return refCount;
    }

private:
    int value;
    ObjectPool<PoolObject>* pool;
    size_t refCount;
};

TEST(ObjectPoolTest, Create)
{
    ObjectPool<PoolObject> pool(10);

    auto obj = pool.get(42);
    obj->addRef();

    ASSERT_EQ(obj->getValue(), 42);

    obj->releaseRef();

    pool.cleanup();
}

constexpr size_t ObjCount = 1000;

TEST(ObjectPoolTest, SpeedNoPool)
{
    std::stack<StandardObject*> objects;

    for (size_t i = 0; i < ObjCount; ++i)
    {
        const auto s = fast_rand() % 2;
        if (s == 0 || objects.empty())
        {
            auto obj = new StandardObject(i);
            obj->addRef();
            objects.push(obj);
        }
        else
        {
            auto obj = objects.top();
            objects.pop();

            obj->releaseRef();
        }
    }

    while (!objects.empty())
    {
        auto obj = objects.top();
        objects.pop();

        obj->releaseRef();
    }
}

TEST(ObjectPoolTest, SpeedWithPool)
{
    ObjectPool<PoolObject> pool(100);

    std::stack<PoolObject*> objects;

    for (size_t i = 0; i < ObjCount; ++i)
    {
        const auto s = fast_rand() % 2;
        if (s == 0 || objects.empty())
        {
            auto obj = pool.get(i);
            obj->addRef();

            objects.push(obj);
        }
        else
        {
            auto obj = objects.top();
            objects.pop();
            obj->releaseRef();
        }
    }

    while (!objects.empty())
    {
        auto obj = objects.top();
        objects.pop();

        obj->releaseRef();
    }

    pool.cleanup();
}
