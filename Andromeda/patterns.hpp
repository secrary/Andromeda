#pragma once

#include <string>
// #include <regex>
#include <algorithm>

#include "utils.hpp"

namespace andromeda
{
    bool is_url(std::string str)
    {
        const std::vector<std::string> urls {"http://", "https://", "ftp://", "ftps://"};
        for (const auto& url : urls)
        {
            if (utils::find_case_insensitive(str, url) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

} // namespace andromeda