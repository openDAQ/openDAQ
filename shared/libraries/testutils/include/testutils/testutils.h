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
#include <gtest/gtest.h>
#include <coretypes/errors.h>
#include <coretypes/errorinfo.h>
#include <coretypes/complex_number_type.h>
#include <coretypes/coretype_traits.h>
#include <type_traits>
#include <testutils/base_test_listener.h>

/*
 *  TEST_F_OPTIONAL(fixture, testName)
 *  TEST_F_OPTIONAL(fixture, testName, false)
 *     - ignore tests if no OPENDAQ_ENABLE_OPTIONAL_TESTS is defined
 *
 *  TEST_F_OPTIONAL(fixture, testName, true)
 *     - run test even if no OPENDAQ_ENABLE_OPTIONAL_TESTS is defined
 */

#ifdef OPENDAQ_ENABLE_OPTIONAL_TESTS
    #define TEST_F_OPTIONAL TEST_F
#else
    // ReSharper disable CppInconsistentNaming
    #define TEST_F_OPTIONAL_true(test_fixture, test_name)  TEST_F(test_fixture, test_name)
    #define TEST_F_OPTIONAL_false(test_fixture, test_name) TEST_F(test_fixture, DISABLED_##test_name)
    #define TEST_F_OPTIONAL(test_fixture, test_name)       TEST_F_OPTIONAL_false(test_fixture, test_name)
    // ReSharper restore CppInconsistentNaming
#endif

#define ASSERT_SUCCEEDED(ErrCode) ASSERT_TRUE(OPENDAQ_SUCCEEDED(ErrCode))

#define ASSERT_THROW_MSG(STATEMENT, EXCEPTION_TYPE, MESSAGE)                                        \
{                                                                                                   \
    std::string message = MESSAGE;                                                                  \
    try                                                                                             \
    {                                                                                               \
        {                                                                                           \
            STATEMENT;                                                                              \
        }                                                                                           \
        FAIL() << "exception '" << #EXCEPTION_TYPE << "' not thrown at all!";                       \
    }                                                                                               \
    catch (const EXCEPTION_TYPE& e)                                                                 \
    {                                                                                               \
        ASSERT_TRUE(std::string(e.what()).find(message) != std::string::npos)                       \
            << "Expected exception \"" << message << "\"" << std::endl                              \
            << "Actually throws \"" << e.what() << "\".";                                           \
    }                                                                                               \
    catch (const std::exception& e)                                                                 \
    {                                                                                               \
        FAIL() << "Expected exception with type " << #EXCEPTION_TYPE << "." << std::endl            \
               << "Actually throws " << GTEST_EXCEPTION_TYPE_(e) << ".";                            \
    }                                                                                               \
    catch (...)                                                                                     \
    {                                                                                               \
        auto ptr = std::current_exception();                                                        \
        ASSERT_THROW((std::rethrow_exception(ptr)), EXCEPTION_TYPE);                                \
    }                                                                                               \
}

#define ASSERT_GENERIC_FLOAT_EQ(T, Val1, Val2)                                                      \
    if constexpr(std::is_same<T, float>::value)                                                     \
        ASSERT_FLOAT_EQ(Val1, Val2);                                                                \
    else if constexpr (std::is_same<T, double>::value)                                              \
        ASSERT_DOUBLE_EQ(Val1, Val2);                                                               \
    else                                                                                            \
        static_assert(daq::DependentFalse<T>::value, "Type compare not supported");

#define ASSERT_GENERIC_VALUE_EQ(T, Val1, Val2)                                                      \
    if constexpr(std::is_same<T, float>::value)                                                     \
        ASSERT_FLOAT_EQ(Val1, Val2);                                                                \
    else if constexpr (std::is_same<T, double>::value)                                              \
        ASSERT_DOUBLE_EQ(Val1, Val2);                                                               \
    else                                                                                            \
        ASSERT_EQ(Val1, Val2); 

inline testing::AssertionResult CmpErrorCodeHelperEQ(const char* lhs_expression,
                                                        const char* rhs_expression, 
                                                        daq::ErrCode lhs,
                                                        daq::ErrCode rhs) 
{
    if (lhs == rhs) 
    {
        if (OPENDAQ_FAILED(lhs))
            daqClearErrorInfo();
        return testing::AssertionSuccess();
    }
    return testing::internal::CmpHelperEQFailure(lhs_expression, rhs_expression, lhs, rhs);
};

#define ASSERT_ERROR_CODE_EQ(lhs, rhs) \
    ASSERT_PRED_FORMAT2(CmpErrorCodeHelperEQ, lhs, rhs)

#define ASSERT_ERROR_CODE_FAILED(errCode)                                                                                 \
    do                                                                                                                    \
    {                                                                                                                     \
        const ErrCode errCode_ = (errCode);                                                                               \
        ASSERT_TRUE(OPENDAQ_FAILED(errCode_)) << "Expected error code to be failed, but it was successful: " << errCode_; \
        daqClearErrorInfo();                                                                                              \
    } while (0)

namespace daq
{
    // // ReSharper disable once CppInconsistentNaming
    template <typename T>
    inline void PrintTo(const Complex_Number<T>& complex, std::ostream* os)
    {
        CoreTypeHelper<Complex_Number<T>>::Print(*os, complex);
    }
}
