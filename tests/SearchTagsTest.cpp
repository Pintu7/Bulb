// Checks for the #tag words of the search box (src/SearchTags.h).
#include "../src/SearchTags.h"
#include <cstdio>

static int failures = 0;

static void Check(bool ok, const char* what)
{
	if (ok == false)
	{
		printf("FAIL: %s\n", what);
		failures++;
	}
}

int main()
{
	using namespace SearchTags;
	using V = std::vector<std::wstring>;

	Check(Parse(L"Lavoro, #server x") == V{ L"lavoro", L"server", L"x" }, "parse splits on commas and spaces, lowers, drops #");
	Check(Parse(L"a A a") == V{ L"a" }, "parse drops duplicates");
	Check(Parse(L"it's \"q\" 50% <b>|") == V{ L"its", L"q", L"50", L"b" }, "parse removes characters that break SQL or markers");
	Check(Parse(L" \t\r\n#'").empty(), "parse of nothing usable is empty");

	Check(TagOf(L"#Lav") == L"lav", "#word names a tag");
	Check(TagOf(L"lav") == L"", "word without # is not a tag");
	Check(TagOf(L"#") == L"", "lone # is not a tag");
	Check(TagOf(L"#'%") == L"", "# with only bad characters is not a tag");

	Check(Words(L"  a  #b c ") == V{ L"a", L"#b", L"c" }, "words skip repeated spaces");

	Check(Toggle(L"hello", L"work") == L"hello #work ", "toggle adds a missing tag at the end");
	Check(Toggle(L"#Work hello", L"work") == L"hello ", "toggle removes the tag in any case");
	Check(Toggle(L"", L"work") == L"#work ", "toggle on an empty search");

	Check(Rename(L"x #old y", L"old", L"new") == L"x #new y ", "rename keeps the word's place");
	Check(Rename(L"x #old y", L"old", L"") == L"x y ", "rename to nothing removes the tag");
	Check(Rename(L"#ol x", L"old", L"new") == L"#ol x ", "rename leaves a prefix of the tag alone");

	Check(KeepMatching(L"#lav #gone text", V{ L"lavoro", L"casa" }) == L"#lav text ", "keep words some tag starts with, drop the rest");
	Check(KeepMatching(L"plain", V{}) == L"plain ", "plain words always stay");

	if (failures == 0)
	{
		printf("SearchTags: all checks passed\n");
	}
	return failures == 0 ? 0 : 1;
}
