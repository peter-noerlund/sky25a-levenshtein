#pragma once

#include <set>
#include <span>
#include <string>
#include <vector>

namespace tt09_levenshtein
{

class TestSet
{
public:
    struct Config
    {
        char minChar = 0;
        char maxChar = 0;
        unsigned int minDictionaryWordLength = 0;
        unsigned int maxDictionaryWordLength = 0;
        unsigned int dictionaryWordCount = 0;
        unsigned int minSearchWordLength = 0;
        unsigned int maxSearchWordLength = 0;
        unsigned int searchWordCount = 0;
    };

    explicit TestSet(const Config& config);

    constexpr std::span<const std::string> dictionaryWords() const noexcept
    {
        return m_dictionaryWords;
    }

    constexpr std::span<const std::string> searchWords() const noexcept
    {
        return m_searchWords;
    }

private:
    static std::vector<std::string> generateWords(
        char minChar,
        char maxChar,
        unsigned int minLength,
        unsigned int maxLength,
        unsigned int count,
        std::set<std::string>& words);

    std::vector<std::string> m_dictionaryWords;
    std::vector<std::string> m_searchWords;
};

} // namespace tt09_levenshtein