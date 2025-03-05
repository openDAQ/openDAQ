#include <stdio.h>

#include "copendaq.h"

int main(){

    List* list = NULL;
    List_createList(&list);

    Integer* i1 = NULL, *i2 = NULL;
    Integer_createInteger(&i1, 5);
    Integer_createInteger(&i2, 6);

    List_pushBack(list, i1);
    List_pushBack(list, i2);

    SizeT size = 0;
    List_getCount(list, &size);
    for(int i = 0; i < size; ++i){
        Integer* integer = NULL;
        Int val = 0;
        List_getItemAt(list, i, &integer);
        Integer_getValue(integer, &val);
        printf("[%d]: %lld\n", i, val);
        BaseObject_releaseRef(integer);
    }
    BaseObject_releaseRef(list);
    BaseObject_releaseRef(i1);
    BaseObject_releaseRef(i2);

	return 0;
}