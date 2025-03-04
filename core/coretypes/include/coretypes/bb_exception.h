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
        : DaqException(false, errCode, fmt::format(format, std::forward<Params>(params)...))
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

#ifndef NDEBUG
    [[nodiscard]]
    DaqException& setFileName(ConstCharPtr fileName)
    {
        this->fileName = fileName;
        return *this;
    }

    [[nodiscard]]
    ConstCharPtr getFileName() const
    {
        return fileName;
    }

    [[nodiscard]]
    DaqException& setFileLine(Int fileLine)
    {
        this->fileLine = fileLine;
        return *this;
    }

    [[nodiscard]]
    Int getFileLine() const
    {
        return fileLine;
    }
#endif

protected:
    template <typename... Params>
    explicit DaqException(bool defaultMsg, ErrCode errCode, const std::string& msg)
        : runtime_error(msg)
        , errCode(errCode)
        , defaultMsg(defaultMsg)
    {
    }

private:
    ErrCode errCode;
    bool defaultMsg;
#ifndef NDEBUG
    ConstCharPtr fileName;
    Int fileLine;
#endif
};

END_NAMESPACE_OPENDAQ
