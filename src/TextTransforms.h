#pragma once

#include <map>
#include <string>

// Text conversions applied at paste time. Each returns the input unchanged when it does not apply
// (not JSON, nothing encoded, not valid Base64 text), so a wrong choice never destroys the clip.
namespace TextTransforms
{
	// indents JSON with two spaces; strings are kept as they are
	std::wstring FormatJson(const std::wstring& text);

	// %20 / + -> space, %C3%A8 -> è (UTF-8)
	std::wstring UrlDecode(const std::wstring& text);

	// standard or URL-safe Base64 whose bytes are UTF-8 text
	std::wstring Base64Decode(const std::wstring& text);

	// replaces {name} with values[name]; unknown {words} stay as typed
	std::wstring ExpandTemplate(const std::wstring& text, const std::map<std::wstring, std::wstring>& values);
}
