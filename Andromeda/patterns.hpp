#pragma once

#include <string>
// #include <regex>
#include <algorithm>

#include "utils.hpp"

namespace andromeda
{
    bool is_url(const std::string& str)
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

    bool is_email(const std::string& text)
    {
        std::vector<std::string> strs{};
        utils::split(text, strs);

        for (const auto& str: strs)
        {
            const auto at_loc = str.find("@");
            if (at_loc != std::string::npos)
            {
                const auto dot_loc = str.find_last_of(".");
                if (dot_loc != std::string::npos && dot_loc > at_loc)
                {
                    const auto is_alpha_mail = std::all_of(str.begin() + dot_loc + 1, str.end(), [](char chr)
                    {
                        return std::isalpha(chr);
                    });
                    return is_alpha_mail;
                }
            }
        }

        return false;
    }

} // namespace andromeda