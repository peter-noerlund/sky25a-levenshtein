#pragma once

#include <string_view>

unsigned int levenshtein(std::string_view s, std::string_view t) noexcept;