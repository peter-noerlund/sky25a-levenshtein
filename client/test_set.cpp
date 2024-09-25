#include "test_set.h"

#include <fmt/format.h>

#include <random>
#include <set>
#include <stdexcept>
#include <utility>

namespace tt09_levenshtein
{

TestSet::TestSet(const Config& config)
{
    std::set<std::string> words;

    m_dictionaryWords = generateWords(
        config.minChar,
        config.maxChar,
        config.minDictionaryWordLength,
        config.maxDictionaryWordLength,
        config.dictionaryWordCount,
        words);

    m_searchWords = generateWords(
        config.minChar,
        config.maxChar,
        config.minSearchWordLength,
        config.maxSearchWordLength,
        config.searchWordCount,
        words);
}

std::vector<std::string> TestSet::generateWords(
    char minChar,
    char maxChar,
    unsigned int minLength,
    unsigned int maxLength,
    unsigned int count,
    std::set<std::string>& words)
{
    const unsigned int maxRetries = 10;

    std::random_device rng;
    std::default_random_engine prng(rng());

    std::uniform_int_distribution<unsigned int> lengthDist(minLength, maxLength);
    std::uniform_int_distribution<char> charDist(minChar, maxChar);

    std::vector<std::string> wordList;
    wordList.reserve(count);

    for (unsigned int i = 0; i != count; ++i)
    {
        std::string word;

        unsigned int retries;
        for (retries = 0; retries != maxRetries; ++retries)
        {
            word.clear();
            auto length = lengthDist(prng);
            word.reserve(length);

            for (std::string::size_type j = 0; j != length; ++j)
            {
                word.push_back(charDist(prng));
            }

            if (!words.contains(word))
            {
                break;
            }
        }

        if (retries == maxRetries)
        {
            throw std::runtime_error(fmt::format("Failed to generate a unique word after {} retries", maxRetries).c_str());
        }

        words.insert(word);
        wordList.push_back(std::move(word));
    }

    return wordList;
}

} // namespace tt09_levenshtein

