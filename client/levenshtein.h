#pragma once

#include <string_view>

namespace tt09_levenshtein
{

unsigned int levenshtein(std::string_view s, std::string_view t) noexcept;

} // namespace tt09_levenshtein