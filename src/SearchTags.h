#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <wchar.h>

// The #tag words of the search box and tag names typed by the user, as plain strings so they can be tested on their own.
namespace SearchTags
{
	inline std::vector<std::wstring> Split(const std::wstring& text, const wchar_t* separators)
	{
		std::vector<std::wstring> words;
		size_t start = text.find_first_not_of(separators);
		while (start != std::wstring::npos)
		{
			size_t end = text.find_first_of(separators, start);
			words.push_back(text.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start));
			start = end == std::wstring::npos ? end : text.find_first_not_of(separators, end);
		}
		return words;
	}

	// "Lavoro, #server x" -> { "lavoro", "server", "x" }: lower case, no duplicates
	inline std::vector<std::wstring> Parse(const std::wstring& text)
	{
		std::wstring words = text;
		std::replace(words.begin(), words.end(), L',', L' ');

		std::vector<std::wstring> names;
		for (std::wstring name : Split(words, L" \t\r\n"))
		{
			// quotes would break the SQL these names go into, the others clash with the list's symbol markers
			for (wchar_t bad : { L'#', L'\'', L'"', L'|', L'<', L'>', L'%' })
			{
				name.erase(std::remove(name.begin(), name.end(), bad), name.end());
			}
			if (name.empty())
			{
				continue;
			}
			_wcslwr_s(&name[0], name.size() + 1);

			if (std::find(names.begin(), names.end(), name) == names.end())
			{
				names.push_back(name);
			}
		}
		return names;
	}

	// the tag a search word names ("#Lav" -> "lav"); "" when the word is not a #tag
	inline std::wstring TagOf(const std::wstring& word)
	{
		if (word.size() < 2 || word[0] != L'#')
		{
			return L"";
		}
		std::vector<std::wstring> names = Parse(word);
		return names.empty() ? L"" : names[0];
	}

	inline std::vector<std::wstring> Words(const std::wstring& search)
	{
		return Split(search, L" ");
	}

	// the search rebuilt word by word; each kept word is followed by a space so typing can go on
	template <class Map>
	std::wstring Rewrite(const std::wstring& search, Map map)
	{
		std::wstring result;
		for (const std::wstring& word : Words(search))
		{
			std::wstring kept = map(word);
			if (kept.empty() == false)
			{
				result += kept + L" ";
			}
		}
		return result;
	}

	// adds #tag to the search, or removes it when it is already there
	inline std::wstring Toggle(const std::wstring& search, const std::wstring& tag)
	{
		bool removed = false;
		std::wstring result = Rewrite(search, [&](const std::wstring& word)
		{
			bool match = TagOf(word) == tag;
			removed = removed || match;
			return match ? std::wstring() : word;
		});
		return removed ? result : result + L"#" + tag + L" ";
	}

	// #from becomes #to; an empty to removes it
	inline std::wstring Rename(const std::wstring& search, const std::wstring& from, const std::wstring& to)
	{
		return Rewrite(search, [&](const std::wstring& word)
		{
			if (TagOf(word) != from)
			{
				return word;
			}
			return to.empty() ? std::wstring() : L"#" + to;
		});
	}

	// #words match by prefix, so a word stays while some tag still starts with it
	inline std::wstring KeepMatching(const std::wstring& search, const std::vector<std::wstring>& names)
	{
		return Rewrite(search, [&](const std::wstring& word)
		{
			std::wstring tag = TagOf(word);
			bool matches = tag.empty() || std::any_of(names.begin(), names.end(), [&](const std::wstring& name) { return name.compare(0, tag.size(), tag) == 0; });
			return matches ? word : std::wstring();
		});
	}
}
