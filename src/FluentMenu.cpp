#include "stdafx.h"
#include "CP_Main.h"
#include "FluentMenu.h"
#include "FluentDialog.h"
#include <commctrl.h>
#include <dwmapi.h>
#include <memory>
#include <vector>

#pragma comment(lib, "comctl32.lib")

namespace
{
	struct Item
	{
		CString text;
		CString accel;
		bool separator = false;
		bool popup = false;
		UINT id = 0;
		ULONG_PTR oldData = 0;
	};

	const UINT_PTR SubclassId = 0xF1E7;
	std::vector<std::unique_ptr<Item>> s_items;
	int s_dpi = 96;
	HBRUSH s_backBrush = NULL;
	COLORREF s_backBrushColor = CLR_INVALID;

	int S(int value) { return MulDiv(value, s_dpi, 96); }

	// Bulb's shared colors (FluentDialog); a menu is a surface of its own on them
	FluentDialog::Palette Colors() { return FluentDialog::ColorsFor(NULL); }
	COLORREF Back() { return Colors().menu; }
	COLORREF Hover() { return Colors().menuHover; }
	COLORREF Ink() { return Colors().ink; }
	COLORREF Dim() { return Colors().dim; }
	COLORREF Disabled() { return Colors().disabled; }
	COLORREF Line() { return Colors().line; }

	HFONT MakeFont(int px, LPCTSTR face)
	{
		return CreateFont(-S(px), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, face);
	}

	struct Fonts
	{
		int dpi = 0;
		HFONT text = NULL;
		HFONT icon = NULL;
		HFONT chevron = NULL;

		void Ensure()
		{
			if (dpi == s_dpi && text != NULL)
			{
				return;
			}
			if (text) DeleteObject(text);
			if (icon) DeleteObject(icon);
			if (chevron) DeleteObject(chevron);

			text = MakeFont(14, ModernUIFontName());
			icon = MakeFont(16, ModernIconFontName());
			chevron = MakeFont(11, ModernIconFontName());
			dpi = s_dpi;
		}
	} s_fonts;

	WCHAR GlyphForId(UINT id)
	{
		switch (id)
		{
		case 32793: case ID__VIEWFULLDESCRIPTION: return 0xE8A1;                 // view full description
		case 32860: return 0xE70F;                                                // edit clip
		case 32867: case ID_MENU_NEWCLIP32937: case ID_FIRST_NEWCLIP: return 0xE710;
		case 32801: case ID_MENU_DELETEGROUP: return 0xE74D;                      // delete
		case 32776: case ID_MENU_PROPERTIES32898: case ID__PROPERTIES: return 0xE946;
		case ID_MENU_COPYIMAGETEXT: return 0xE8C8;                                // copy
		case ID_MENU_PASTEQUEUE: return 0xE8FD;                                   // list
		case ID_MENU_EDITTAGS: return 0xE8EC;                                     // tag
		case ID_MENU_FILTERON: return 0xE71C;
		case ID_MENU_SHOWSTARREDCLIPS: return 0xE734;
		case ID_MENU_GOTOENTRY: return 0xE8AD;
		case 32842: case ID_FIRST_HELP: return 0xE897;
		case 32806: case ID_FIRST_EXIT: case ID_FIRST_EXIT_NO_DB: return 0xE7E8;  // power
		case 32805: case ID_FIRST_OPTION: case ID_FIRST_OPTIONS: return 0xE713;  // settings
		case 32775: case ID_FIRST_TOGGLECONNECTCV: return 0xE71B;                 // link
		case ID_FIRST_SAVECURRENTCLIPBOARD: case ID_MENU_SAVECURRENTCLIPBOARD: case ID__SAVETOFILE: return 0xE74E;
		case ID_MENU_IMPORTCLIP32935: case ID_FIRST_IMPORT: return 0xE8B5;
		case ID_FIRST_SHOWQUICKPASTE: return 0xE8A7;
		case ID_MENU_GLOBALHOTKEYS32933: case ID_FIRST_GLOBALHOTKEYS: return 0xE765;  // keyboard
		case ID_MENU_NEWGROUP32896: return 0xE8F4;                                // new folder
		}
		return 0;
	}

	// submenus have no command id: they are known by their English name, a translated one is looked up first
	WCHAR GlyphForPopup(const CString& shown)
	{
		CString text = theApp.m_Language.EnglishMenuText(shown);
		static const struct { LPCTSTR name; WCHAR glyph; } popups[] =
		{
			{ _T("Special Paste"), 0xE77F }, { _T("Tags"), 0xE8EC }, { _T("Groups"), 0xE8B7 }, { _T("More"), 0xE712 },
			{ _T("Send To"), 0xE724 }, { _T("Pause Recording"), 0xE769 }, { _T("Search Options"), 0xE721 },
			{ _T("Quick Options"), 0xE771 }, { _T("Database"), 0xE8F1 }, { _T("Import/Export"), 0xE8B5 },
			{ _T("Compare"), 0xE8EF }, { _T("Clip Order"), 0xE8CB }, { _T("Quick Properties"), 0xE946 },
		};
		for (auto& p : popups)
		{
			if (text == p.name)
			{
				return p.glyph;
			}
		}
		return 0;
	}

	Item* Find(ULONG_PTR data)
	{
		for (auto& item : s_items)
		{
			if ((ULONG_PTR)item.get() == data)
			{
				return item.get();
			}
		}
		return nullptr;
	}

	void Prepare(HMENU menu, bool recursive)
	{
		int count = GetMenuItemCount(menu);
		for (int i = 0; i < count; i++)
		{
			MENUITEMINFO mii = { sizeof(mii) };
			mii.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_ID | MIIM_DATA | MIIM_STRING;
			if (GetMenuItemInfo(menu, i, TRUE, &mii) == FALSE)
			{
				continue;
			}

			if (recursive && mii.hSubMenu != NULL)
			{
				Prepare(mii.hSubMenu, true);
			}

			auto readText = [&](Item* item)
			{
				CString text;
				MENUITEMINFO str = { sizeof(str) };
				str.fMask = MIIM_STRING;
				GetMenuItemInfo(menu, i, TRUE, &str);
				str.cch++;
				str.dwTypeData = text.GetBuffer(str.cch);
				GetMenuItemInfo(menu, i, TRUE, &str);
				text.ReleaseBuffer();

				int tab = text.Find(_T('\t'));
				item->text = tab >= 0 ? text.Left(tab) : text;
				item->accel = tab >= 0 ? text.Mid(tab + 1) : _T("");
			};

			Item* existing = (mii.fType & MFT_OWNERDRAW) ? Find(mii.dwItemData) : nullptr;
			if (existing != nullptr)
			{
				// the owner may have changed the text (shortcut keys are added while the menu opens)
				if (existing->separator == false)
				{
					readText(existing);
				}
				continue;
			}
			if (mii.fType & (MFT_BITMAP | MFT_OWNERDRAW))
			{
				continue; // someone else's drawing, leave it alone
			}

			auto item = std::make_unique<Item>();
			item->separator = (mii.fType & MFT_SEPARATOR) != 0;
			item->popup = mii.hSubMenu != NULL;
			item->id = mii.wID;
			item->oldData = mii.dwItemData;

			if (item->separator == false)
			{
				readText(item.get());
			}

			MENUITEMINFO set = { sizeof(set) };
			set.fMask = MIIM_FTYPE | MIIM_DATA;
			GetMenuItemInfo(menu, i, TRUE, &set);
			set.fType |= MFT_OWNERDRAW;
			set.dwItemData = (ULONG_PTR)item.get();
			SetMenuItemInfo(menu, i, TRUE, &set);

			s_items.push_back(std::move(item));
		}
	}

	void Restore(HMENU menu)
	{
		int count = GetMenuItemCount(menu);
		for (int i = 0; i < count; i++)
		{
			MENUITEMINFO mii = { sizeof(mii) };
			mii.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_DATA;
			if (GetMenuItemInfo(menu, i, TRUE, &mii) == FALSE)
			{
				continue;
			}
			if (mii.hSubMenu != NULL)
			{
				Restore(mii.hSubMenu);
			}
			Item* item = (mii.fType & MFT_OWNERDRAW) ? Find(mii.dwItemData) : nullptr;
			if (item != nullptr)
			{
				mii.fMask = MIIM_FTYPE | MIIM_DATA;
				mii.fType &= ~MFT_OWNERDRAW;
				mii.dwItemData = item->oldData;
				SetMenuItemInfo(menu, i, TRUE, &mii);
			}
		}

		MENUINFO info = { sizeof(info) };
		info.fMask = MIM_BACKGROUND;
		info.hbrBack = NULL;
		SetMenuInfo(menu, &info);
	}

	bool Measure(MEASUREITEMSTRUCT* m)
	{
		Item* item = m->CtlType == ODT_MENU ? Find(m->itemData) : nullptr;
		if (item == nullptr)
		{
			return false;
		}

		if (item->separator)
		{
			m->itemHeight = S(9);
			m->itemWidth = 0;
			return true;
		}

		s_fonts.Ensure();
		HDC dc = GetDC(NULL);
		HGDIOBJ old = SelectObject(dc, s_fonts.text);
		SIZE text = { 0 }, accel = { 0 };
		GetTextExtentPoint32(dc, item->text, item->text.GetLength(), &text);
		if (item->accel.IsEmpty() == FALSE)
		{
			GetTextExtentPoint32(dc, item->accel, item->accel.GetLength(), &accel);
		}
		SelectObject(dc, old);
		ReleaseDC(NULL, dc);

		// icon column + text + gap + shortcut + chevron column
		int width = S(12) + S(16) + S(12) + text.cx + (accel.cx > 0 ? S(40) + accel.cx : 0) + S(12) + S(16);
		m->itemWidth = max(width, S(180));
		m->itemHeight = S(32);
		return true;
	}

	void FillRounded(HDC dc, const RECT& rect, int radius, COLORREF color)
	{
		Gdiplus::Graphics g(dc);
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		Gdiplus::GraphicsPath path;
		Gdiplus::REAL d = (Gdiplus::REAL)radius * 2, l = (Gdiplus::REAL)rect.left, t = (Gdiplus::REAL)rect.top;
		Gdiplus::REAL r = (Gdiplus::REAL)rect.right - 1, b = (Gdiplus::REAL)rect.bottom - 1;
		path.AddArc(l, t, d, d, 180, 90);
		path.AddArc(r - d, t, d, d, 270, 90);
		path.AddArc(r - d, b - d, d, d, 0, 90);
		path.AddArc(l, b - d, d, d, 90, 90);
		path.CloseFigure();
		Gdiplus::SolidBrush brush(Gdiplus::Color(255, GetRValue(color), GetGValue(color), GetBValue(color)));
		g.FillPath(&brush, &path);
	}

	void Paint(HDC dc, RECT r, const Item* item, UINT itemState)
	{
		SetBkColor(dc, Back());
		ExtTextOut(dc, 0, 0, ETO_OPAQUE, &r, NULL, 0, NULL);

		if (item->separator)
		{
			RECT line = { r.left, (r.top + r.bottom) / 2, r.right, (r.top + r.bottom) / 2 + 1 };
			SetBkColor(dc, Line());
			ExtTextOut(dc, 0, 0, ETO_OPAQUE, &line, NULL, 0, NULL);
			return;
		}

		s_fonts.Ensure();
		bool disabled = (itemState & (ODS_DISABLED | ODS_GRAYED)) != 0;
		bool selected = (itemState & ODS_SELECTED) != 0;

		if (selected && disabled == false)
		{
			RECT hover = { r.left + S(4), r.top + S(2), r.right - S(4), r.bottom - S(2) };
			FillRounded(dc, hover, S(4), Hover());
		}

		SetBkMode(dc, TRANSPARENT);
		COLORREF ink = disabled ? Disabled() : Ink();

		WCHAR glyph = (itemState & ODS_CHECKED) ? 0xE73E : (item->popup ? GlyphForPopup(item->text) : GlyphForId(item->id));
		if (glyph != 0)
		{
			SelectObject(dc, s_fonts.icon);
			SetTextColor(dc, ink);
			RECT icon = { r.left + S(12), r.top, r.left + S(12) + S(16), r.bottom };
			DrawTextW(dc, &glyph, 1, &icon, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}

		SelectObject(dc, s_fonts.text);
		SetTextColor(dc, ink);
		RECT text = { r.left + S(40), r.top, r.right - S(28), r.bottom };
		UINT prefix = (itemState & ODS_NOACCEL) ? DT_HIDEPREFIX : 0;
		DrawText(dc, item->text, -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | prefix);

		if (item->accel.IsEmpty() == FALSE)
		{
			SetTextColor(dc, disabled ? Disabled() : Dim());
			DrawText(dc, item->accel, -1, &text, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}

		if (item->popup)
		{
			WCHAR chevron = 0xE76C;
			SelectObject(dc, s_fonts.chevron);
			SetTextColor(dc, disabled ? Disabled() : Dim());
			RECT arrow = { r.right - S(28), r.top, r.right - S(12), r.bottom };
			DrawTextW(dc, &chevron, 1, &arrow, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}
	}

	bool Draw(DRAWITEMSTRUCT* d)
	{
		Item* item = d->CtlType == ODT_MENU ? Find(d->itemData) : nullptr;
		if (item == nullptr)
		{
			return false;
		}

		// painted off screen and copied in one go: painted on screen, the row showed blank between the hover and the text
		RECT r = d->rcItem;
		int width = r.right - r.left, height = r.bottom - r.top;
		HDC buffer = CreateCompatibleDC(d->hDC);
		HBITMAP bitmap = CreateCompatibleBitmap(d->hDC, width, height);
		if (buffer != NULL && bitmap != NULL)
		{
			HGDIOBJ old = SelectObject(buffer, bitmap);
			Paint(buffer, { 0, 0, width, height }, item, d->itemState);
			BitBlt(d->hDC, r.left, r.top, width, height, buffer, 0, 0, SRCCOPY);
			SelectObject(buffer, old);
		}
		else
		{
			Paint(d->hDC, r, item, d->itemState);
		}
		if (bitmap != NULL) DeleteObject(bitmap);
		if (buffer != NULL) DeleteDC(buffer);

		// stops the system from drawing its own submenu arrow on top
		ExcludeClipRect(d->hDC, r.left, r.top, r.right, r.bottom);
		return true;
	}

	LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		switch (msg)
		{
		case WM_MEASUREITEM:
			if (Measure((MEASUREITEMSTRUCT*)lParam))
			{
				return TRUE;
			}
			break;
		case WM_DRAWITEM:
			if (Draw((DRAWITEMSTRUCT*)lParam))
			{
				return TRUE;
			}
			break;
		case WM_INITMENUPOPUP:
		{
			// the owner may add or rename items here; draw those too
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			Prepare((HMENU)wParam, false);
			return result;
		}
		case WM_MENUCHAR:
		{
			// owner-drawn items get no automatic access keys: match the letter after & ourselves ("&Edit" -> E)
			HMENU menu = (HMENU)lParam;
			WCHAR key = (WCHAR)towupper((WCHAR)LOWORD(wParam));
			int count = GetMenuItemCount(menu);
			for (int i = 0; i < count; i++)
			{
				MENUITEMINFO mii = { sizeof(mii) };
				mii.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_STATE;
				if (GetMenuItemInfo(menu, i, TRUE, &mii) == FALSE || (mii.fState & MFS_DISABLED))
				{
					continue;
				}
				Item* item = (mii.fType & MFT_OWNERDRAW) ? Find(mii.dwItemData) : nullptr;
				if (item == nullptr || item->separator)
				{
					continue;
				}
				for (int at = item->text.Find(_T('&')); at >= 0 && at + 1 < item->text.GetLength(); at = item->text.Find(_T('&'), at + 2))
				{
					if (item->text[at + 1] != _T('&'))
					{
						if ((WCHAR)towupper(item->text[at + 1]) == key)
						{
							return MAKELRESULT(i, MNC_EXECUTE);
						}
						break;
					}
				}
			}
			break;
		}
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	void CALLBACK OnMenuPopup(HWINEVENTHOOK, DWORD, HWND hwnd, LONG, LONG, DWORD, DWORD)
	{
		DWORD round = 2; // DWMWCP_ROUND
		DwmSetWindowAttribute(hwnd, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &round, sizeof(round));
		COLORREF border = Line();
		DwmSetWindowAttribute(hwnd, 34 /*DWMWA_BORDER_COLOR*/, &border, sizeof(border));
	}
}

UINT FluentMenu::Track(HMENU menu, UINT flags, int x, int y, HWND owner)
{
	// one menu at a time owns the drawing state; a menu opened while another is up is shown plainly
	// rather than clearing the items the open one is still drawing with
	static bool tracking = false;
	if (tracking)
	{
		return ::TrackPopupMenu(menu, flags, x, y, 0, owner, NULL);
	}
	struct Guard { Guard() { tracking = true; } ~Guard() { tracking = false; } } guard;

	s_dpi = GetDpiForWindow(owner);
	if (s_dpi <= 0)
	{
		s_dpi = 96;
	}

	s_items.clear();
	Prepare(menu, true);

	if (s_backBrushColor != Back())
	{
		if (s_backBrush) DeleteObject(s_backBrush);
		s_backBrushColor = Back();
		s_backBrush = CreateSolidBrush(s_backBrushColor);
	}
	MENUINFO info = { sizeof(info) };
	info.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
	info.hbrBack = s_backBrush;
	SetMenuInfo(menu, &info);

	SetWindowSubclass(owner, OwnerProc, SubclassId, 0);
	HWINEVENTHOOK hook = SetWinEventHook(EVENT_SYSTEM_MENUPOPUPSTART, EVENT_SYSTEM_MENUPOPUPSTART, NULL, OnMenuPopup,
		GetCurrentProcessId(), GetCurrentThreadId(), WINEVENT_OUTOFCONTEXT);

	UINT result = ::TrackPopupMenu(menu, flags, x, y, 0, owner, NULL);

	if (hook != NULL)
	{
		UnhookWinEvent(hook);
	}
	RemoveWindowSubclass(owner, OwnerProc, SubclassId);

	if (IsMenu(menu))
	{
		Restore(menu);
	}
	s_items.clear();

	return result;
}
