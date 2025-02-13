#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    struct Number;
    typedef struct Number Number;

    void EXPORTED Number_getFloatValue(Number* self, Float* value);
    void EXPORTED Number_getIntValue(Number* self, Int* value);

#ifdef __cplusplus
}
#endif
