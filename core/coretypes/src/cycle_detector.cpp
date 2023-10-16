#include <coretypes/cycle_detector.h>
#include <unordered_set>
#include <cassert>

using namespace daq;

static thread_local std::unordered_set<IBaseObject*>* objects = nullptr;

int daqCycleDetectEnter(IBaseObject* object)
{
    if (objects == nullptr)
        objects = new std::unordered_set<IBaseObject*>();

    const auto it = objects->find(object);
    if (it != objects->end())
        return 0;

    objects->insert(object);
    return 1;
}

void daqCycleDetectLeave(IBaseObject* object)
{
    if (objects == nullptr)
    {
        assert(false);
        return;
    }

    [[maybe_unused]] auto objectsErased = objects->erase(object);
    assert(objectsErased == 1);

    if (objects->empty())
    {
        delete objects;
        objects = nullptr;
    }
}
