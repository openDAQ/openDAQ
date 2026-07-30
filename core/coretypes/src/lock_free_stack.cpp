#include <coretypes/lock_free_stack.h>
#include <coretypes/validation.h>
#include <boost/lockfree/stack.hpp>
#include <atomic>

using namespace daq;

struct LockFreeStack
{
    LockFreeStack(size_t initialSize)
        : stack(initialSize)
    {
    }

    boost::lockfree::stack<void*> stack;
};

LockFreeStackHandle daqLockFreeStackCreate(size_t initialSize)
{
    return new LockFreeStack(initialSize);
}   

void daqLockFreeStackPush(LockFreeStackHandle handle, LockFreeStackElement element)
{
    auto* s = static_cast<LockFreeStack*>(handle);
    s->stack.push(element);
}

LockFreeStackElement daqLockFreeStackPop(LockFreeStackHandle handle)
{
    auto* s = static_cast<LockFreeStack*>(handle);
    LockFreeStackElement element = nullptr;
    if (s->stack.pop(element))
        return element;
    return nullptr;
}

void daqLockFreeStackDestroy(LockFreeStackHandle handle)
{
    delete static_cast<LockFreeStack*>(handle);
}

