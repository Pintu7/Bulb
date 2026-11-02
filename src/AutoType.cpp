#include "stdafx.h"
#include "AutoType.h"
#include "SendKeys.h"
#include <atomic>
#include <thread>
#include <vector>

// Keys go out as the local layout's real keys with their scancodes, the way a keyboard sends them:
// remote sessions (TeamViewer, Remote Desktop) pass scancodes on and often drop KEYEVENTF_UNICODE packets,
// which are only the fallback for characters the layout has no key for. See .scratch/autotype/research.
namespace
{
	std::atomic<bool> s_typing(false);

	bool Pressed(int vk)
	{
		return (GetAsyncKeyState(vk) & 0x8000) != 0;
	}

	void Send(WORD vk, WORD scan, DWORD flags)
	{
		INPUT input = {};
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = vk;
		input.ki.wScan = scan;
		input.ki.dwFlags = flags;
		SendInput(1, &input, sizeof(INPUT));
	}

	void Key(WORD vk, bool up, HKL layout)
	{
		UINT scan = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC_EX, layout);
		DWORD flags = up ? KEYEVENTF_KEYUP : 0;
		if ((scan & 0xFF00) == 0xE000)
		{
			flags |= KEYEVENTF_EXTENDEDKEY;
		}
		Send(vk, (WORD)(scan & 0xFF), flags);
	}

	void Unicode(wchar_t unit)
	{
		Send(0, unit, KEYEVENTF_UNICODE);
		Send(0, unit, KEYEVENTF_UNICODE | KEYEVENTF_KEYUP);
	}

	// the modifiers a character needs, from VkKeyScan's high byte; AltGr (Ctrl+Alt) goes out as left Ctrl + right Alt
	std::vector<WORD> Modifiers(BYTE shiftState)
	{
		std::vector<WORD> keys;
		if ((shiftState & 6) == 6)
		{
			keys.push_back(VK_LCONTROL);
			keys.push_back(VK_RMENU);
		}
		else
		{
			if (shiftState & 2) keys.push_back(VK_LCONTROL);
			if (shiftState & 4) keys.push_back(VK_LMENU);
		}
		if (shiftState & 1) keys.push_back(VK_LSHIFT);
		return keys;
	}

	void TypeCharacter(const std::wstring& text, size_t& i, HKL layout, DWORD modifierPause)
	{
		wchar_t c = text[i];
		if (c >= 0xD800 && c <= 0xDBFF && i + 1 < text.size())
		{
			// a character outside the basic plane (an emoji): no key types it
			Unicode(c);
			Unicode(text[++i]);
			return;
		}

		WORD vk = 0;
		BYTE shiftState = 0;
		if (c == L'\n')
		{
			vk = VK_RETURN;
		}
		else if (c == L'\t')
		{
			vk = VK_TAB;
		}
		else
		{
			SHORT scan = VkKeyScanExW(c, layout);
			bool deadKey = scan != -1 && (MapVirtualKeyExW(LOBYTE(scan), MAPVK_VK_TO_CHAR, layout) & 0x80000000) != 0;
			if (scan == -1 || (HIBYTE(scan) & ~7) != 0 || deadKey)
			{
				// no plain key on this layout (or one that waits for the next key, like ^ on some layouts)
				Unicode(c);
				return;
			}
			vk = LOBYTE(scan);
			shiftState = HIBYTE(scan);
		}

		std::vector<WORD> modifiers = Modifiers(shiftState);
		for (WORD m : modifiers)
		{
			Key(m, false, layout);
		}
		if (!modifiers.empty()) Sleep(modifierPause);
		Key(vk, false, layout);
		Key(vk, true, layout);
		if (!modifiers.empty()) Sleep(modifierPause);
		for (auto m = modifiers.rbegin(); m != modifiers.rend(); ++m)
		{
			Key(*m, true, layout);
		}
	}

	void Type(std::wstring text, DWORD delayMs)
	{
		// the shortcut's Ctrl+Alt are still down: typed now, every letter would be an AltGr combination
		const int held[] = { VK_CONTROL, VK_MENU, VK_SHIFT, VK_LWIN, VK_RWIN };
		ULONGLONG giveUp = GetTickCount64() + 2000;
		auto anyHeld = [&]() { for (int vk : held) if (Pressed(vk)) return true; return false; };
		while (anyHeld() && GetTickCount64() < giveUp)
		{
			Sleep(20);
		}
		if (anyHeld())
		{
			CSendKeys().AllKeysUp();
		}
		Sleep(50);

		HWND target = GetForegroundWindow();
		HKL layout = GetKeyboardLayout(GetWindowThreadProcessId(target, NULL));
		DWORD modifierPause = min(delayMs, (DWORD)50);

		for (size_t i = 0; i < text.size(); i++)
		{
			if (Pressed(VK_ESCAPE) || GetForegroundWindow() != target)
			{
				break; // stopped, or the user moved to another window: never type into it
			}
			TypeCharacter(text, i, layout, modifierPause);
			Sleep(delayMs);
		}
		s_typing = false;
	}
}

bool AutoType::Start(const std::wstring& text, unsigned delayMs)
{
	if (s_typing.exchange(true))
	{
		return false;
	}
	std::thread(Type, text, (DWORD)delayMs).detach();
	return true;
}
