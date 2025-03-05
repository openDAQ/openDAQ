#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    typedef struct List List;

    void EXPORTED List_getItemAt(List* self, SizeT index, BaseObject** obj);
    void EXPORTED List_getCount(List* self, SizeT* size);
    void EXPORTED List_setItemAt(List* self, SizeT index, BaseObject* obj);
    void EXPORTED List_pushBack(List* self, BaseObject* obj);
    void EXPORTED List_pushFront(List* self, BaseObject* obj);
    void EXPORTED List_moveBack(List* self, BaseObject* obj);
    void EXPORTED List_moveFront(List* self, BaseObject* obj);
    void EXPORTED List_popBack(List* self, BaseObject** obj);
    void EXPORTED List_popFront(List* self, BaseObject** obj);
    void EXPORTED List_insertAt(List* self, SizeT index, BaseObject* obj);
    void EXPORTED List_removeAt(List* self, SizeT index, BaseObject** obj);
    void EXPORTED List_deleteAt(List* self, SizeT index);
    void EXPORTED List_clear(List* self);

    // void List_createStartIterator(IIterator** iterator);
    // void List_createEndIterator(IIterator** iterator);

    void EXPORTED List_create(List** list);
    // void List_createWithElementType(IntfID id);

#ifdef __cplusplus
}
#endif