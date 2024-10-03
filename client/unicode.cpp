#include "unicode.h"

#include <unicode/unistr.h>
#include <unicode/schriter.h>
#include <unicode/stringpiece.h>

namespace tt09_levenshtein
{

std::u32string Unicode::toUTF32(std::string_view string)
{
    auto unicodeString = icu::UnicodeString::fromUTF8(icu::StringPiece(string.data(), static_cast<std::int32_t>(string.size())));
    std::u32string buffer;
    buffer.resize(unicodeString.length());
    UErrorCode ec = {};
    auto res = unicodeString.toUTF32(reinterpret_cast<UChar32*>(buffer.data()), static_cast<std::int32_t>(buffer.size()), ec);
    buffer.resize(res);

    return buffer;
}


} // namespace tt09_levenshtein