// Checks what the Digitazione automatica types for a given clipboard text.
#include <cstdio>
#include <string>

namespace AutoType { bool Start(const std::wstring&, unsigned) { return false; } }
#include "../src/AutoType.h"

static int failures = 0;

static void Check(const wchar_t* name, const std::wstring& clipboard, size_t limit, const std::wstring& typed, size_t total)
{
	AutoType::Prepared p = AutoType::Prepare(clipboard, limit);
	if (p.text != typed || p.total != total)
	{
		failures++;
		wprintf(L"FAIL %s: typed '%s' (%zu), expected '%s' (%zu)\n", name, p.text.c_str(), p.total, typed.c_str(), total);
	}
}

int main()
{
	Check(L"plain text", L"Prova è", 100, L"Prova è", 7);
	Check(L"CRLF becomes one line break", L"a\r\nb", 100, L"a\nb", 3);
	Check(L"lone CR becomes a line break", L"a\rb", 100, L"a\nb", 3);
	Check(L"final line breaks are not typed", L"dir\r\n\r\n", 100, L"dir", 3);
	Check(L"tabs stay", L"a\tb", 100, L"a\tb", 3);
	Check(L"limit cuts and counts the whole text", L"abcdef", 4, L"abcd", 6);
	Check(L"limit counts after dropping the final break", L"abcd\r\n", 4, L"abcd", 4);
	Check(L"an emoji is one character", L"a\U0001F600b", 2, L"a\U0001F600", 3);
	Check(L"an emoji is never cut in half", L"\U0001F600\U0001F600", 1, L"\U0001F600", 2);
	Check(L"empty clipboard", L"", 100, L"", 0);
	Check(L"only line breaks", L"\r\n\r\n", 100, L"", 0);
	Check(L"limit zero types nothing", L"abc", 0, L"", 3);

	if (failures == 0)
	{
		wprintf(L"AutoType: all checks pass\n");
	}
	return failures == 0 ? 0 : 1;
}
