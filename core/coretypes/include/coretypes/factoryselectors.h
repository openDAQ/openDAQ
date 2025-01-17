/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#define OPENDAQ_FACTORY_SELECTOR(_0, _1, _1_x, _2, _2_x, _3, _3_x, _4, _4_x, _5, _5_x, _6, _6_x, _7, _7_x, _8, _8_x, NAME, ...) NAME

#define OPENDAQ_FACTORY_TYPES_SELECTOR(_TYPE, _NAME) _TYPE
#define OPENDAQ_FACTORY_TYPES_SELECTOR_PAR(_TYPE, _NAME) OPENDAQ__STRIP_PAR _TYPE
#define OPENDAQ_FACTORY_NAMES_SELECTOR(_TYPE, _NAME) _NAME
#define OPENDAQ_FACTORY_TYPES_AND_NAMES_SELECTOR(_TYPE, _NAME) _TYPE _NAME
#define OPENDAQ_FACTORY_TYPES_AND_NAMES_SELECTOR_PAR(_TYPE, _NAME) OPENDAQ__STRIP_PAR _TYPE _NAME

#define OPENDAQ_INTERNAL_EXPAND(x) x

#if !defined(__RSCPP_VERSION) && defined(_MSC_VER) && !defined(__clang__)

    #define OPENDAQ_FACTORY_EXP_0(SELECTOR, unsued)
    #define OPENDAQ_FACTORY_EXP_1(SELECTOR, unused, _TYPE, _NAME) , SELECTOR(_TYPE, _NAME)
    #define OPENDAQ_FACTORY_EXP_2(SELECTOR, unused, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_1(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_3(SELECTOR, unused, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_2(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_4(SELECTOR, unused, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_3(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_5(SELECTOR, unused, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_4(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_6(SELECTOR, unused, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_5(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_7(SELECTOR, unused, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_6(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_8(SELECTOR, unused, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_7(SELECTOR, unused, __VA_ARGS__))

    #define OPENDAQ_FACTORY_EXP_CM_0(SELECTOR, unsued)
    #define OPENDAQ_FACTORY_EXP_CM_1(SELECTOR, unused, _TYPE, _NAME) SELECTOR(_TYPE, _NAME)
    #define OPENDAQ_FACTORY_EXP_CM_2(SELECTOR, unused, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME),  OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_CM_1(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_CM_3(SELECTOR, unused, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME),  OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_CM_2(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_CM_4(SELECTOR, unused, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME),  OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_CM_3(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_CM_5(SELECTOR, unused, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME),  OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_CM_4(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_CM_6(SELECTOR, unused, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME),  OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_CM_5(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_CM_7(SELECTOR, unused, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME),  OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_CM_6(SELECTOR, unused, __VA_ARGS__))
    #define OPENDAQ_FACTORY_EXP_CM_8(SELECTOR, unused, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME),  OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_EXP_CM_7(SELECTOR, unused, __VA_ARGS__))

    #define OPENDAQ_INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__

    #define OPENDAQ_INTERNAL_FACTORY_EXP_PRIVATE(SELECTOR, CM, ...)     \
        OPENDAQ_INTERNAL_EXPAND(OPENDAQ_FACTORY_SELECTOR(##__VA_ARGS__, \
                                                     CM##_8,            \
                                                     8_unused,          \
                                                     CM##_7,            \
                                                     7_unused,          \
                                                     CM##_6,            \
                                                     6_unused,          \
                                                     CM##_5,            \
                                                     5_unused,          \
                                                     CM##_4,            \
                                                     4_unused,          \
                                                     CM##_3,            \
                                                     3_unused,          \
                                                     CM##_2,            \
                                                     2_unused,          \
                                                     CM##_1,            \
                                                     1_unused,          \
                                                     CM##_0))           \
            OPENDAQ_INTERNAL_EXPAND((SELECTOR, __VA_ARGS__))

    #define OPENDAQ_FACTORY_TYPES(...) OPENDAQ_INTERNAL_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_SELECTOR, OPENDAQ_FACTORY_EXP, OPENDAQ_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
    #define OPENDAQ_FACTORY_TYPES_PAR(...) OPENDAQ_INTERNAL_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_SELECTOR_PAR, OPENDAQ_FACTORY_EXP, OPENDAQ_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
    #define OPENDAQ_FACTORY_NAMES(...) OPENDAQ_INTERNAL_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_NAMES_SELECTOR, OPENDAQ_FACTORY_EXP, OPENDAQ_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
    #define OPENDAQ_FACTORY_TYPES_AND_NAMES(...) OPENDAQ_INTERNAL_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_AND_NAMES_SELECTOR, OPENDAQ_FACTORY_EXP, OPENDAQ_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
    #define OPENDAQ_FACTORY_TYPES_AND_NAMES_PAR(...) OPENDAQ_INTERNAL_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_AND_NAMES_SELECTOR_PAR, OPENDAQ_FACTORY_EXP, OPENDAQ_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))

    #define OPENDAQ_FACTORY_CM_TYPES(...) OPENDAQ_INTERNAL_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_SELECTOR, OPENDAQ_FACTORY_EXP_CM, OPENDAQ_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
    #define OPENDAQ_FACTORY_CM_NAMES(...) OPENDAQ_INTERNAL_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_NAMES_SELECTOR, OPENDAQ_FACTORY_EXP_CM, OPENDAQ_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
    #define OPENDAQ_FACTORY_CM_TYPES_AND_NAMES(...) OPENDAQ_INTERNAL_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_AND_NAMES_SELECTOR, OPENDAQ_FACTORY_EXP_CM, OPENDAQ_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))
    #define OPENDAQ_FACTORY_CM_TYPES_AND_NAMES_PAR(...) OPENDAQ_INTERNAL_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_AND_NAMES_SELECTOR_PAR, OPENDAQ_FACTORY_EXP_CM, OPENDAQ_INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))

#else // _MSC_VER

    #define OPENDAQ_FACTORY_EXP_0(SELECTOR)
    #define OPENDAQ_FACTORY_EXP_1(SELECTOR, _TYPE, _NAME) , SELECTOR(_TYPE, _NAME)
    #define OPENDAQ_FACTORY_EXP_2(SELECTOR, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_FACTORY_EXP_1(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_3(SELECTOR, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_FACTORY_EXP_2(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_4(SELECTOR, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_FACTORY_EXP_3(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_5(SELECTOR, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_FACTORY_EXP_4(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_6(SELECTOR, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_FACTORY_EXP_5(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_7(SELECTOR, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_FACTORY_EXP_6(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_8(SELECTOR, _TYPE, _NAME, ...) , SELECTOR(_TYPE, _NAME) OPENDAQ_FACTORY_EXP_7(SELECTOR, __VA_ARGS__)

    #define OPENDAQ_FACTORY_EXP_CM_0(SELECTOR)
    #define OPENDAQ_FACTORY_EXP_CM_1(SELECTOR, _TYPE, _NAME) SELECTOR(_TYPE, _NAME)
    #define OPENDAQ_FACTORY_EXP_CM_2(SELECTOR, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME), OPENDAQ_FACTORY_EXP_CM_1(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_CM_3(SELECTOR, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME), OPENDAQ_FACTORY_EXP_CM_2(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_CM_4(SELECTOR, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME), OPENDAQ_FACTORY_EXP_CM_3(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_CM_5(SELECTOR, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME), OPENDAQ_FACTORY_EXP_CM_4(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_CM_6(SELECTOR, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME), OPENDAQ_FACTORY_EXP_CM_5(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_CM_7(SELECTOR, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME), OPENDAQ_FACTORY_EXP_CM_6(SELECTOR, __VA_ARGS__)
    #define OPENDAQ_FACTORY_EXP_CM_8(SELECTOR, _TYPE, _NAME, ...) SELECTOR(_TYPE, _NAME), OPENDAQ_FACTORY_EXP_CM_7(SELECTOR, __VA_ARGS__)

    #define OPENDAQ_FACTORY_EXP_PRIVATE(SELECTOR, CM, ...) \
        OPENDAQ_FACTORY_SELECTOR(unused,                   \
                             ##__VA_ARGS__,                \
                             CM##_8,                       \
                             unused,                       \
                             CM##_7,                       \
                             unused,                       \
                             CM##_6,                       \
                             unused,                       \
                             CM##_5,                       \
                             unused,                       \
                             CM##_4,                       \
                             unused,                       \
                             CM##_3,                       \
                             unused,                       \
                             CM##_2,                       \
                             unused,                       \
                             CM##_1,                       \
                             unused,                       \
                             CM##_0)                       \
            (SELECTOR, ##__VA_ARGS__)

    #define OPENDAQ_FACTORY_TYPES(...) OPENDAQ_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_SELECTOR, OPENDAQ_FACTORY_EXP, ## __VA_ARGS__)
    #define OPENDAQ_FACTORY_TYPES_PAR(...) OPENDAQ_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_SELECTOR_PAR, OPENDAQ_FACTORY_EXP, ## __VA_ARGS__)
    #define OPENDAQ_FACTORY_NAMES(...) OPENDAQ_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_NAMES_SELECTOR, OPENDAQ_FACTORY_EXP, ## __VA_ARGS__)
    #define OPENDAQ_FACTORY_TYPES_AND_NAMES(...) OPENDAQ_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_AND_NAMES_SELECTOR, OPENDAQ_FACTORY_EXP, ## __VA_ARGS__)
    #define OPENDAQ_FACTORY_TYPES_AND_NAMES_PAR(...) OPENDAQ_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_AND_NAMES_SELECTOR_PAR, OPENDAQ_FACTORY_EXP, ## __VA_ARGS__)

    #define OPENDAQ_FACTORY_CM_TYPES(...) OPENDAQ_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_SELECTOR, OPENDAQ_FACTORY_EXP_CM, ## __VA_ARGS__)
    #define OPENDAQ_FACTORY_CM_NAMES(...) OPENDAQ_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_NAMES_SELECTOR, OPENDAQ_FACTORY_EXP_CM, ## __VA_ARGS__)
    #define OPENDAQ_FACTORY_CM_TYPES_AND_NAMES(...) OPENDAQ_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_AND_NAMES_SELECTOR, OPENDAQ_FACTORY_EXP_CM, ## __VA_ARGS__)
    #define OPENDAQ_FACTORY_CM_TYPES_AND_NAMES_PAR(...) OPENDAQ_FACTORY_EXP_PRIVATE(OPENDAQ_FACTORY_TYPES_AND_NAMES_SELECTOR_PAR, OPENDAQ_FACTORY_EXP_CM, ## __VA_ARGS__)

#endif // _MSC_VER

#define OPENDAQ_FACTORY_NAMES_EX(...) OPENDAQ_FACTORY_NAMES(__VA_ARGS__)

/*  Testing

OPENDAQ_FACTORY_TYPES()
OPENDAQ_FACTORY_TYPES(type1, name1)
OPENDAQ_FACTORY_TYPES(type1, name1, type2, name2)
OPENDAQ_FACTORY_TYPES(type1, name1, type2, name2, type3, name3)
OPENDAQ_FACTORY_TYPES(type1, name1, type2, name2, type3, name3, type4, name4)
OPENDAQ_FACTORY_TYPES(type1, name1, type2, name2, type3, name3, type4, name4, type5, name5)

---------------

OPENDAQ_FACTORY_NAMES()
OPENDAQ_FACTORY_NAMES(type1, name1)
OPENDAQ_FACTORY_NAMES(type1, name1, type2, name2)
OPENDAQ_FACTORY_NAMES(type1, name1, type2, name2, type3, name3)
OPENDAQ_FACTORY_NAMES(type1, name1, type2, name2, type3, name3, type4, name4)
OPENDAQ_FACTORY_NAMES(type1, name1, type2, name2, type3, name3, type4, name4, type5, name5)

---------------

OPENDAQ_FACTORY_TYPES_AND_NAMES()
OPENDAQ_FACTORY_TYPES_AND_NAMES(type1, name1)
OPENDAQ_FACTORY_TYPES_AND_NAMES(type1, name1, type2, name2)
OPENDAQ_FACTORY_TYPES_AND_NAMES(type1, name1, type2, name2, type3, name3)
OPENDAQ_FACTORY_TYPES_AND_NAMES(type1, name1, type2, name2, type3, name3, type4, name4)
OPENDAQ_FACTORY_TYPES_AND_NAMES(type1, name1, type2, name2, type3, name3, type4, name4, type5, name5)

*/
