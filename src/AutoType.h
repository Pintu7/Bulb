#pragma once

#include <string>

// Digitazione automatica: types the clipboard text into the active window as key presses,
// for windows that refuse a paste (a remote session).
namespace AutoType
{
	struct Prepared
	{
		std::wstring text; // what gets typed: line breaks as '\n', none at the end, at most the limit
		size_t total = 0;  // characters before the limit was applied
	};

	// Counts characters, not UTF-16 units: an emoji is one character and is never cut in half.
	inline Prepared Prepare(const std::wstring& clipboard, size_t limit)
	{
		std::wstring text;
		text.reserve(clipboard.size());
		for (size_t i = 0; i < clipboard.size(); i++)
		{
			wchar_t c = clipboard[i];
			if (c == L'\r')
			{
				text += L'\n';
				if (i + 1 < clipboard.size() && clipboard[i + 1] == L'\n')
				{
					i++;
				}
			}
			else if (c != L'\0')
			{
				text += c;
			}
		}
		// a final line break would press Enter: a copied command must not run by itself
		while (!text.empty() && text.back() == L'\n')
		{
			text.pop_back();
		}

		Prepared result;
		size_t end = text.size();
		for (size_t i = 0; i < text.size(); i++)
		{
			bool secondHalf = text[i] >= 0xDC00 && text[i] <= 0xDFFF && i > 0 && text[i - 1] >= 0xD800 && text[i - 1] <= 0xDBFF;
			if (secondHalf)
			{
				continue;
			}
			if (result.total == limit)
			{
				end = i;
			}
			result.total++;
		}
		result.text = text.substr(0, end);
		return result;
	}

	// Starts typing 'text' into the foreground window on a worker thread, 'delayMs' between characters;
	// Esc stops it. False when a typing is already running.
	bool Start(const std::wstring& text, unsigned delayMs);
}
