#pragma once

#include <algorithm>

#include <optional>
#include <string>

#include <httpparser/request.h>

namespace daq::ws_streaming
{
    /**
     * Gets the value of a header from an httpparser::Request object.
     *
     * @param request A reference to the request object.
     * @param name The name of the header (case-sensitive).
     *
     * @return The value of the header, or std::nullopt if the header is not set.
     */
    inline std::optional<std::string> get_header(const httpparser::Request& request, const char *name)
    {
        auto it = std::find_if(
            request.headers.begin(),
            request.headers.end(),
            [name](const httpparser::Request::HeaderItem& h) { return h.name == name; });

        if (it == request.headers.end())
            return {};

        return it->value;
    }
}
