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
#include <coretypes/common.h>
#include <fmt/format.h>

BEGIN_NAMESPACE_OPENDAQ

class DaqException : public std::runtime_error
{
public:
    template <typename... Params>
    explicit DaqException(ErrCode errCode, const std::string& format, Params&&... params)
        : DaqException(false, errCode, fmt::format(fmt::runtime(format), std::forward<Params>(params)...))
    {
    }

    [[nodiscard]]
    ErrCode getErrCode() const
    {
        return errCode;
    }

    [[nodiscard]]
    bool getDefaultMsg() const
    {
        return defaultMsg;
    }

    [[nodiscard]]
    ConstCharPtr getFileName() const
    {
        return fileName;
    }

    [[nodiscard]]
    Int getFileLine() const
    {
        return fileLine;
    }

protected:
    template <typename... Params>
    explicit DaqException(bool defaultMsg, ErrCode errCode, const std::string& msg, ConstCharPtr fileName = nullptr, Int fileLine = -1)
        : runtime_error(msg)
        , errCode(errCode)
        , defaultMsg(defaultMsg)
        , fileName(fileName)
        , fileLine(fileLine)
    {
    }

    template <typename... Params>
    explicit DaqException(ConstCharPtr fileName, Int fileLine, ErrCode errCode, const std::string& format, Params&&... params)
        : DaqException(false, errCode, fmt::format(fmt::runtime(format), std::forward<Params>(params)...), fileName, fileLine)
    {
    }

private:
    ErrCode errCode;
    bool defaultMsg;
    ConstCharPtr fileName;
    Int fileLine;
};

END_NAMESPACE_OPENDAQ
