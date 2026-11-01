// Checks for TextTransforms. Build and run: tests\run_tests.bat
#include "..\src\TextTransforms.h"
#include <cstdio>

static int failures = 0;

static void Expect(const wchar_t* what, const std::wstring& got, const std::wstring& expected)
{
	if (got != expected)
	{
		failures++;
		wprintf(L"FAIL %ls:\n  got      [%ls]\n  expected [%ls]\n", what, got.c_str(), expected.c_str());
	}
}

int main()
{
	using namespace TextTransforms;

	Expect(L"json object", FormatJson(L"{\"a\":1,\"b\":[1,2],\"c\":{}}"),
		L"{\n  \"a\": 1,\n  \"b\": [\n    1,\n    2\n  ],\n  \"c\": {}\n}");
	Expect(L"json keeps strings", FormatJson(L"{\"t\":\"a, b: {c}\"}"), L"{\n  \"t\": \"a, b: {c}\"\n}");
	Expect(L"json escaped quote", FormatJson(L"[\"x\\\"y\"]"), L"[\n  \"x\\\"y\"\n]");
	Expect(L"not json", FormatJson(L"hello, world"), L"hello, world");
	Expect(L"broken json stays", FormatJson(L"{\"a\":1"), L"{\"a\":1");

	Expect(L"url decode", UrlDecode(L"caff%C3%A8+e%20latte%21"), L"caffè e latte!");
	Expect(L"url nothing to do", UrlDecode(L"plain text"), L"plain text");
	Expect(L"url bad percent", UrlDecode(L"100%"), L"100%");
	Expect(L"url keeps emoji", UrlDecode(L"a%20b\U0001F600c"), L"a b\U0001F600c");

	Expect(L"base64", Base64Decode(L"Y2lhbyBtb25kbw=="), L"ciao mondo");
	Expect(L"base64 no padding", Base64Decode(L"Y2lhbw"), L"ciao");
	Expect(L"base64 url-safe utf8", Base64Decode(L"Y2FmZsOo"), L"caffè");
	Expect(L"not base64", Base64Decode(L"hello world!"), L"hello world!");
	Expect(L"binary stays", Base64Decode(L"AAECAw=="), L"AAECAw==");

	Expect(L"template", ExpandTemplate(L"Roma, {date} - {name} {unknown}", { { L"date", L"30/10/2026" }, { L"name", L"Andrea" } }),
		L"Roma, 30/10/2026 - Andrea {unknown}");
	Expect(L"template unclosed", ExpandTemplate(L"a {date", { { L"date", L"x" } }), L"a {date");

	if (failures == 0)
	{
		wprintf(L"TextTransforms: all checks pass\n");
	}
	return failures == 0 ? 0 : 1;
}
