#include "levenshtein.h"

#include <algorithm>
#include <vector>

namespace tt09_levenshtein
{

unsigned int levenshtein(std::u32string_view s, std::u32string_view t) noexcept
{
	auto m = s.size();
	auto n = t.size();
	auto d = std::vector<std::vector<unsigned int>>(n + 1, std::vector<unsigned int>(m + 1));

	d[0][0] = 0;
	for (unsigned int j = 1; j <= n; ++j)
		d[j][0] = j;
	for (unsigned int i = 1; i <= m; ++i)
		d[0][i] = i;

	for (unsigned int j = 1; j <= n; ++j)
	{
		for (unsigned int i = 1; i <= m; ++i)
		{
			auto cost = (s[i - 1] == t[j - 1] ? 0 : 1);
			d[j][i] = std::min(std::min(d[j - 1][i], d[j][i - 1]) + 1, d[j - 1][i - 1] + cost);
		}
	}

	return d[n][m];
}

} // namespace tt09_levenshtein