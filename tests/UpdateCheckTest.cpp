// Checks how Bulb reads GitHub's release list and compares versions.
#include "../src/UpdateCheck.h"
#include <cstdio>

void UpdateCheck::Start(HWND, UINT) {}

static int failures = 0;
static void Check(bool ok, const char* what)
{
	if (!ok)
	{
		printf("FAIL: %s\n", what);
		failures++;
	}
}

int main()
{
	// the newest release comes first; the author's html_url and other fields come before tag_name
	std::string list = R"([{"url":"https://api.github.com/repos/Pintu7/Bulb/releases/1","html_url":"https://github.com/Pintu7/Bulb/releases/tag/v0.9.1","id":1,"author":{"login":"Pintu7"},"tag_name":"v0.9.1","prerelease":true},{"tag_name":"v0.9.0"}])";
	Check(UpdateCheck::NewestTag(list) == "v0.9.1", "reads the first tag in the list");
	Check(UpdateCheck::NewestTag(R"([{"tag_name" : "1.0"}])") == "1.0", "allows spaces around the colon");
	Check(UpdateCheck::NewestTag("[]") == "", "no releases gives no tag");
	Check(UpdateCheck::NewestTag(R"({"message":"API rate limit exceeded"})") == "", "an error answer gives no tag");
	Check(UpdateCheck::NewestTag(R"([{"tag_name":"v1.0.0/../../evil"}])") == "", "a tag that is not a plain version is refused");
	Check(UpdateCheck::NewestTag(R"([{"tag_name":"v1.0.0-beta"}])") == "", "a tag with a suffix is refused");
	Check(UpdateCheck::NewestTag(R"([{"tag_name":"v1)") == "", "a cut-off answer gives no tag");

	int v[3];
	Check(UpdateCheck::Parse("v0.9.0", v) && v[0] == 0 && v[1] == 9 && v[2] == 0, "parses v0.9.0");
	Check(UpdateCheck::Parse("1.2", v) && v[2] == 0, "a missing patch is 0");
	Check(!UpdateCheck::Parse("1", v), "one number alone is not a version");
	Check(!UpdateCheck::Parse("1.2.3.4", v), "four numbers are refused");
	Check(!UpdateCheck::Parse("1..2", v), "an empty part is refused");
	Check(!UpdateCheck::Parse("1.2.", v), "a trailing dot is refused");
	Check(!UpdateCheck::Parse("", v), "empty is refused");

	Check(UpdateCheck::IsNewer("v0.9.1", 0, 9, 0), "0.9.1 is newer than 0.9.0");
	Check(UpdateCheck::IsNewer("v0.10.0", 0, 9, 5), "0.10.0 is newer than 0.9.5, compared as numbers");
	Check(UpdateCheck::IsNewer("v1.0.0", 0, 9, 9), "1.0.0 is newer than 0.9.9");
	Check(!UpdateCheck::IsNewer("v0.9.0", 0, 9, 0), "the same version is not newer");
	Check(!UpdateCheck::IsNewer("v0.8.9", 0, 9, 0), "an older version is not newer");
	Check(!UpdateCheck::IsNewer("garbage", 0, 9, 0), "an unreadable tag is not newer");

	if (failures == 0)
	{
		printf("UpdateCheck: all checks passed\n");
	}
	return failures == 0 ? 0 : 1;
}
