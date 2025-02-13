#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    struct Integer;
    typedef struct Integer Integer;

    void EXPORTED Integer_getValue(Integer* self, Int* value);
    void EXPORTED Integer_equalsValue(Integer* self, Int value, Bool* equals);
    void EXPORTED Integer_create(Integer** integer, Int value);

#ifdef __cplusplus
}
#endif