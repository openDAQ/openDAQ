#include "list.h"

#include <opendaq/opendaq.h>

void List_getItemAt(List* self, SizeT index, BaseObject** obj)
{
    reinterpret_cast<daq::IList*>(self)->getItemAt(index, reinterpret_cast<daq::IBaseObject**>(obj));
}

void List_getCount(List* self, SizeT* size)
{
    reinterpret_cast<daq::IList*>(self)->getCount(size);
}

void List_setItemAt(List* self, SizeT index, BaseObject* obj)
{
    reinterpret_cast<daq::IList*>(self)->setItemAt(index, reinterpret_cast<daq::IBaseObject*>(obj));
}

void List_pushBack(List* self, BaseObject* obj)
{
    reinterpret_cast<daq::IList*>(self)->pushBack(reinterpret_cast<daq::IBaseObject*>(obj));
}

void List_pushFront(List* self, BaseObject* obj)
{
    reinterpret_cast<daq::IList*>(self)->pushFront(reinterpret_cast<daq::IBaseObject*>(obj));
}

void List_moveBack(List* self, BaseObject* obj)
{
    reinterpret_cast<daq::IList*>(self)->moveBack(reinterpret_cast<daq::IBaseObject*>(obj));
}

void List_moveFront(List* self, BaseObject* obj)
{
    reinterpret_cast<daq::IList*>(self)->moveFront(reinterpret_cast<daq::IBaseObject*>(obj));
}

void List_popBack(List* self, BaseObject** obj)
{
    reinterpret_cast<daq::IList*>(self)->popBack(reinterpret_cast<daq::IBaseObject**>(obj));
}

void List_popFront(List* self, BaseObject** obj)
{
    reinterpret_cast<daq::IList*>(self)->popFront(reinterpret_cast<daq::IBaseObject**>(obj));
}

void List_insertAt(List* self, SizeT index, BaseObject* obj)
{
    reinterpret_cast<daq::IList*>(self)->insertAt(index, reinterpret_cast<daq::IBaseObject*>(obj));
}

void List_removeAt(List* self, SizeT index, BaseObject** obj)
{
    reinterpret_cast<daq::IList*>(self)->removeAt(index, reinterpret_cast<daq::IBaseObject**>(obj));
}

void List_deleteAt(List* self, SizeT index)
{
    reinterpret_cast<daq::IList*>(self)->deleteAt(index);
}

void List_clear(List* self)
{
    reinterpret_cast<daq::IList*>(self)->clear();
}

// void List_createStartIterator(IIterator** iterator);
// void List_createEndIterator(IIterator** iterator);

void List_create(List** list)
{
    *list = reinterpret_cast<List*>(daq::List_Create());
}
// void List_createWithElementType(IntfID id);