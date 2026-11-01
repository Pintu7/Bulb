// built without the precompiled header so the test program can compile it on its own
#include "TextTransforms.h"
#include <windows.h>
#include <string>
#include <vector>

namespace TextTransforms
{
	static std::wstring Utf8ToWide(const std::string& bytes, bool& valid)
	{
		valid = true;
		if (bytes.empty())
		{
			return L"";
		}

		int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), nullptr, 0);
		if (length <= 0)
		{
			valid = false;
			return L"";
		}

		std::wstring wide(length, L'\0');
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), &wide[0], length);
		return wide;
	}

	std::wstring FormatJson(const std::wstring& text)
	{
		size_t first = text.find_first_not_of(L" \t\r\n");
		if (first == std::wstring::npos || (text[first] != L'{' && text[first] != L'['))
		{
			return text;
		}

		std::wstring out;
		int depth = 0;
		bool inString = false;
		auto newline = [&]() { out += L"\n"; out.append(depth * 2, L' '); };

		for (size_t i = first; i < text.size(); i++)
		{
			wchar_t c = text[i];
			if (inString)
			{
				out += c;
				if (c == L'\\' && i + 1 < text.size())
				{
					out += text[++i];
				}
				else if (c == L'"')
				{
					inString = false;
				}
				continue;
			}

			switch (c)
			{
			case L'"': inString = true; out += c; break;
			case L'{': case L'[':
				out += c;
				// keep {} and [] on one line
				if (i + 1 < text.size() && text.find_first_not_of(L" \t\r\n", i + 1) != std::wstring::npos &&
					(text[text.find_first_not_of(L" \t\r\n", i + 1)] == L'}' || text[text.find_first_not_of(L" \t\r\n", i + 1)] == L']'))
				{
					i = text.find_first_not_of(L" \t\r\n", i + 1);
					out += text[i];
					break;
				}
				depth++;
				newline();
				break;
			case L'}': case L']':
				depth = depth > 0 ? depth - 1 : 0;
				newline();
				out += c;
				break;
			case L',': out += c; newline(); break;
			case L':': out += L": "; break;
			case L' ': case L'\t': case L'\r': case L'\n': break;
			default: out += c; break;
			}
		}

		// unbalanced brackets or an open string: not JSON after all
		return (depth == 0 && inString == false) ? out : text;
	}

	std::wstring UrlDecode(const std::wstring& text)
	{
		std::string bytes;
		bool changed = false;
		for (size_t i = 0; i < text.size(); i++)
		{
			wchar_t c = text[i];
			if (c == L'%' && i + 2 < text.size() && iswxdigit(text[i + 1]) && iswxdigit(text[i + 2]))
			{
				bytes += (char)std::stoi(text.substr(i + 1, 2), nullptr, 16);
				i += 2;
				changed = true;
			}
			else if (c == L'+')
			{
				bytes += ' ';
				changed = true;
			}
			else
			{
				// re-encode the character as UTF-8 so it joins the decoded bytes; an emoji is two wchar_t (a surrogate pair)
				// that only convert together
				int units = (IS_HIGH_SURROGATE(c) && i + 1 < text.size() && IS_LOW_SURROGATE(text[i + 1])) ? 2 : 1;
				char utf8[8] = {};
				int length = WideCharToMultiByte(CP_UTF8, 0, &text[i], units, utf8, sizeof(utf8), nullptr, nullptr);
				bytes.append(utf8, length);
				i += units - 1;
			}
		}

		bool valid = true;
		std::wstring decoded = changed ? Utf8ToWide(bytes, valid) : text;
		return valid ? decoded : text;
	}

	std::wstring Base64Decode(const std::wstring& text)
	{
		std::wstring compact;
		for (wchar_t c : text)
		{
			if (c == L'-') compact += L'+';        // URL-safe alphabet
			else if (c == L'_') compact += L'/';
			else if (c != L' ' && c != L'\r' && c != L'\n' && c != L'\t') compact += c;
		}
		while (compact.size() % 4 != 0)
		{
			compact += L'=';
		}

		auto value = [](wchar_t c) -> int
		{
			if (c >= L'A' && c <= L'Z') return c - L'A';
			if (c >= L'a' && c <= L'z') return c - L'a' + 26;
			if (c >= L'0' && c <= L'9') return c - L'0' + 52;
			if (c == L'+') return 62;
			if (c == L'/') return 63;
			return -1;
		};

		if (compact.size() < 4)
		{
			return text;
		}

		std::string bytes;
		for (size_t i = 0; i < compact.size(); i += 4)
		{
			int a = value(compact[i]), b = value(compact[i + 1]);
			int c = compact[i + 2] == L'=' ? -2 : value(compact[i + 2]);
			int d = compact[i + 3] == L'=' ? -2 : value(compact[i + 3]);
			if (a < 0 || b < 0 || c == -1 || d == -1 || (c == -2 && d != -2) || (i + 4 < compact.size() && (c == -2 || d == -2)))
			{
				return text; // not Base64
			}

			bytes += (char)((a << 2) | (b >> 4));
			if (c >= 0) bytes += (char)(((b & 15) << 4) | (c >> 2));
			if (d >= 0) bytes += (char)(((c & 3) << 6) | d);
		}

		bool valid = true;
		std::wstring decoded = Utf8ToWide(bytes, valid);
		if (valid == false)
		{
			return text;
		}

		// binary that happens to be valid UTF-8 still holds control characters
		for (wchar_t c : decoded)
		{
			if (c < 32 && c != L'\r' && c != L'\n' && c != L'\t')
			{
				return text;
			}
		}
		return decoded;
	}

	std::wstring ExpandTemplate(const std::wstring& text, const std::map<std::wstring, std::wstring>& values)
	{
		std::wstring out;
		for (size_t i = 0; i < text.size(); i++)
		{
			if (text[i] == L'{')
			{
				size_t close = text.find(L'}', i);
				if (close != std::wstring::npos)
				{
					auto found = values.find(text.substr(i + 1, close - i - 1));
					if (found != values.end())
					{
						out += found->second;
						i = close;
						continue;
					}
				}
			}
			out += text[i];
		}
		return out;
	}
}
