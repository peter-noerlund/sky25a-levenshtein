#pragma once

#include <string_view>

namespace tt09_levenshtein
{

unsigned int levenshtein(std::u32string_view s, std::u32string_view t) noexcept;

} // namespace tt09_levenshtein