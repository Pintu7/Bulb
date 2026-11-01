// Checks for PasteQueueKeys: which key presses run the paste queue. Build and run: tests\run_tests.bat
#include "..\src\PasteQueueKeys.h"
#include <cstdio>

static int failures = 0;

static void Expect(const wchar_t* what, PasteQueueKeys::Result got, PasteQueueKeys::Result expected)
{
	if (got != expected)
	{
		failures++;
		wprintf(L"FAIL %ls: got %d, expected %d\n", what, (int)got, (int)expected);
	}
}

const unsigned LeftCtrl = 0xA2, RightCtrl = 0xA3, V = 'V';
using R = PasteQueueKeys::Result;

int main()
{
	{
		PasteQueueKeys keys;
		keys.OnKey(LeftCtrl, true, false, false);
		Expect(L"Ctrl+V pastes the next clip", keys.OnKey(V, true, false, false), R::PasteNext);
		Expect(L"releasing V is swallowed", keys.OnKey(V, false, false, false), R::Swallow);
		Expect(L"V again with Ctrl still held pastes again", keys.OnKey(V, true, false, false), R::PasteNext);
	}
	{
		PasteQueueKeys keys;
		keys.OnKey(LeftCtrl, true, false, false);
		keys.OnKey(V, true, false, false);
		Expect(L"holding V does not run through the queue", keys.OnKey(V, true, false, false), R::Swallow);
	}
	{
		PasteQueueKeys keys;
		keys.OnKey(LeftCtrl, true, false, false);
		keys.OnKey(V, true, false, false);
		keys.OnKey(V, false, false, false);
		Expect(L"Bulb's own Ctrl release is ignored", keys.OnKey(LeftCtrl, false, true, false), R::Pass);
		Expect(L"so the user's held Ctrl still counts", keys.OnKey(V, true, false, false), R::PasteNext);
	}
	{
		PasteQueueKeys keys;
		keys.OnKey(LeftCtrl, true, false, false);
		Expect(L"Ctrl+Shift+V passes through", keys.OnKey(V, true, false, true), R::Pass);
	}
	{
		PasteQueueKeys keys;
		Expect(L"V alone is typed", keys.OnKey(V, true, false, false), R::Pass);
	}
	{
		PasteQueueKeys keys;
		keys.OnKey(RightCtrl, true, false, false);
		Expect(L"right Ctrl works too", keys.OnKey(V, true, false, false), R::PasteNext);
	}
	{
		PasteQueueKeys keys;
		keys.OnKey(LeftCtrl, true, false, false);
		keys.OnKey(LeftCtrl, false, false, false);
		Expect(L"after the user lets go of Ctrl, V is typed", keys.OnKey(V, true, false, false), R::Pass);
	}
	{
		PasteQueueKeys keys;
		keys.OnKey(LeftCtrl, true, false, false);
		Expect(L"Bulb's own Ctrl+V passes", keys.OnKey(V, true, true, false), R::Pass);
	}

	if (failures == 0)
	{
		wprintf(L"PasteQueueKeys: all checks pass\n");
	}
	return failures == 0 ? 0 : 1;
}
