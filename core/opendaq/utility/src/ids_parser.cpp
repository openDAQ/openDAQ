#include <opendaq/ids_parser.h>

BEGIN_NAMESPACE_OPENDAQ

bool IdsParser::splitRelativeId(const std::string& id, std::string& start, std::string& rest)
{
    const auto equalsIdx = id.find_first_of('/');
    if (std::string::npos != equalsIdx)
    {
        start = id.substr(0, equalsIdx);
        rest = id.substr(equalsIdx + 1);
        return true;
    }

    return false;
}

END_NAMESPACE_OPENDAQ
