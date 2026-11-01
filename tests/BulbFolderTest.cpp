// Checks for BulbFolder: where a new database goes, and which folders may be encrypted. Build and run: tests\run_tests.bat
#include "..\src\BulbFolder.h"
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

static void ExpectTrue(const wchar_t* what, bool value, bool expected)
{
	if (value != expected)
	{
		failures++;
		wprintf(L"FAIL %ls: got %d, expected %d\n", what, value, expected);
	}
}

int main()
{
	using namespace BulbFolder;
	const std::wstring appData = L"C:\\Users\\me\\AppData\\Roaming\\Clips\\";

	Expect(L"user folder gets a Bulb folder", Place(L"D:\\Docs\\Bulb.db", appData), L"D:\\Docs\\Bulb\\Bulb.db");
	Expect(L"Bulb folder stays", Place(L"D:\\Docs\\Bulb\\Bulb.db", appData), L"D:\\Docs\\Bulb\\Bulb.db");
	Expect(L"Bulb folder any case", Place(L"D:\\Docs\\bulb\\x.db", appData), L"D:\\Docs\\bulb\\x.db");
	Expect(L"app data folder stays", Place(L"c:\\users\\ME\\appdata\\roaming\\clips\\Bulb.db", appData), L"c:\\users\\ME\\appdata\\roaming\\clips\\Bulb.db");
	Expect(L"portable bare name stays", Place(L"Bulb.db", appData), L"Bulb.db");
	Expect(L"placing twice changes nothing", Place(Place(L"D:\\Docs\\Bulb.db", appData), appData), L"D:\\Docs\\Bulb\\Bulb.db");

	ExpectTrue(L"user folder is never Bulb's", IsOwn(L"D:\\Docs\\", appData), false);
	ExpectTrue(L"a name ending in Bulb is not Bulb", IsOwn(L"D:\\MyBulb\\", appData), false);
	ExpectTrue(L"Bulb folder is Bulb's", IsOwn(L"D:\\Docs\\Bulb\\", appData), true);
	ExpectTrue(L"app data without slash", IsOwn(L"C:\\Users\\me\\AppData\\Roaming\\Clips", appData), true);
	ExpectTrue(L"drive root is not Bulb's", IsOwn(L"D:\\", appData), false);

	if (failures == 0)
	{
		wprintf(L"BulbFolder: all checks pass\n");
	}
	return failures == 0 ? 0 : 1;
}
