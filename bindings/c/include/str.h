#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    typedef struct String String;

    void EXPORTED String_getCharPtr(String* self, ConstCharPtr* value);
    void EXPORTED String_getLength(String* self, SizeT* size);
    void EXPORTED String_create(String** str, ConstCharPtr cstr);

#ifdef __cplusplus
}
#endif