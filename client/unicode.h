#pragma once

#include <string>

namespace tt09_levenshtein
{

class Unicode
{
public:
    static std::u32string toUTF32(std::string_view word);
};

} // namespace tt09_levenshtein