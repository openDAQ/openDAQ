#include "opcuaclient/opcuatimertaskcontextcollection.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

TimerTaskContextCollection::TimerTaskContextCollection()
{
}

void* TimerTaskContextCollection::createContext()
{
    return new KeyType(this);
}

void TimerTaskContextCollection::deleteContext(void* context)
{
    delete static_cast<KeyType*>(context);
}

void TimerTaskContextCollection::insertTimerTask(void* context, OpcUaCallbackIdent ident, const OpcUaTimerTaskType& task)
{
    assert(context != nullptr);
    auto keyType = static_cast<KeyType*>(context);
    assert(keyType->owner == this);

    keyType->ident = ident;

    taskCallbacks.insert(std::pair<KeyType*, OpcUaTimerTaskType>(keyType, task));
}

void TimerTaskContextCollection::removeTimerTask(OpcUaCallbackIdent ident)
{
    KeyType keyType;
    keyType.ident = ident;

    auto it = taskCallbacks.find(&keyType);
    if (it != taskCallbacks.end())
    {
        KeyType* kt = it->first;
        taskCallbacks.erase(&keyType);
        deleteContext(kt);
    }
}

bool TimerTaskContextCollection::timerTaskExists(OpcUaCallbackIdent ident) const
{
    KeyType keyType;
    keyType.ident = ident;

    return taskCallbacks.find(&keyType) != taskCallbacks.end();
}

void TimerTaskContextCollection::getTaskExecData(void* context,
                                                 OpcUaCallbackIdent* callbackIdent,
                                                 OpcUaTimerTaskType** task)
{
    KeyType* keyType = static_cast<KeyType*>(context);

    *callbackIdent = keyType->ident;
    *task = &keyType->owner->taskCallbacks[keyType];
}

END_NAMESPACE_OPENDAQ_OPCUA
