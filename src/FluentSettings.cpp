// The Windows 11 Settings page layout: FluentDialog::SettingsLayout turns a classic options page into rows on cards,
// a scrolling page with section headings. The themed controls themselves are drawn by FluentDialog.cpp.
#include "stdafx.h"
#include "CP_Main.h"
#include "FluentDialogInternal.h"
#include <commctrl.h>
#include <Uxtheme.h>
#include <map>
#include <vector>

namespace FluentInternal
{
	struct Placement
	{
		HWND hwnd;
		RECT rect;     // page coordinates with no scroll
		int height;    // the size given to the window (a combo box's includes its open list)
	};

	struct LayoutState
	{
		std::vector<FluentDialog::SettingsLayout::Entry> entries;
		std::vector<HWND> titles;
		std::vector<HWND> descriptions;
		std::map<int, SIZE> designed;   // control id -> its size in the dialog template
		std::vector<Placement> placements;
		std::vector<RECT> rows;
		HFONT titleFont = NULL;
		HFONT descriptionFont = NULL;
		HFONT sectionFont = NULL;
		UINT fontDpi = 0;
		int scroll = 0;
		int content = 0;
		bool laying = false;
	};

	LayoutState* StateOf(HWND page)
	{
		return (LayoutState*)GetProp(page, LayoutProp);
	}

	HFONT MakeFont(HWND page, int size, int weight)
	{
		return CreateFont(-Scale(page, size), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH, ModernUIFontName());
	}

	HWND MakeLabel(HWND page, const CString& text, HFONT font, bool wrap)
	{
		HWND label = CreateWindowEx(0, _T("Static"), text, WS_CHILD | WS_VISIBLE | SS_NOPREFIX | (wrap ? SS_LEFT : SS_LEFT | SS_ENDELLIPSIS),
			0, 0, 0, 0, page, (HMENU)(INT_PTR)0xFFFF, (HINSTANCE)GetWindowLongPtr(page, GWLP_HINSTANCE), NULL);
		SendMessage(label, WM_SETFONT, (WPARAM)font, FALSE);
		return label;
	}

	bool IsClass(HWND hwnd, LPCTSTR name)
	{
		TCHAR cls[64] = { 0 };
		GetClassName(hwnd, cls, 64);
		return _tcsicmp(cls, name) == 0;
	}

	bool IsToggle(HWND hwnd)
	{
		LONG type = GetWindowLong(hwnd, GWL_STYLE) & BS_TYPEMASK;
		return IsClass(hwnd, _T("Button")) && (type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON);
	}

	// the visible height Windows 11 gives each kind of control in a settings row
	int RowControlHeight(HWND page, HWND control, SIZE designed)
	{
		if (IsClass(control, WC_COMBOBOX)) return designed.cy;
		if (IsClass(control, HOTKEY_CLASS)) return Scale(page, 32);
		if (IsClass(control, _T("Edit"))) return max(designed.cy, Scale(page, 24)); // a single line field keeps its text centred at its own height
		if (IsClass(control, _T("Static"))) return Scale(page, 20);
		if (IsClass(control, _T("Button")))
		{
			if (IsToggle(control)) return Scale(page, 28);
			LONG type = GetWindowLong(control, GWL_STYLE) & BS_TYPEMASK;
			if (type == BS_OWNERDRAW) return Scale(page, 24); // a link
			return Scale(page, 32);
		}
		return designed.cy;
	}

	// works out where everything goes, with no scroll; returns the content height
	int Measure(HWND page, LayoutState* state, int width)
	{
		state->placements.clear();
		state->rows.clear();

		int pad = Scale(page, 16);
		int gap = Scale(page, 8);
		int y = 0;
		HDC dc = GetDC(page);
		HGDIOBJ oldFont = SelectObject(dc, state->descriptionFont);

		for (size_t i = 0; i < state->entries.size(); i++)
		{
			FluentDialog::SettingsLayout::Entry& entry = state->entries[i];
			if (entry.section)
			{
				y += i == 0 ? 0 : Scale(page, 20);
				state->placements.push_back({ state->titles[i], { Scale(page, 2), y, width - Scale(page, 2), y + Scale(page, 22) }, Scale(page, 22) });
				y += Scale(page, 30);
				continue;
			}

			// control widths: as designed, a set width, or the room the title leaves
			std::vector<int> widths;
			int fixed = 0;
			int sideCount = 0;
			bool hasFill = false;
			int belowHeight = 0;
			for (const auto& item : entry.controls)
			{
				SIZE designed = state->designed[item.id];
				int w = item.width == FluentDialog::SettingsLayout::Keep ? designed.cx : (item.width > 0 ? Scale(page, item.width) : 0);
				widths.push_back(w);
				if (item.width == FluentDialog::SettingsLayout::Below)
				{
					belowHeight += designed.cy + gap;
					continue;
				}
				sideCount++;
				fixed += w;
				hasFill = hasFill || item.width == FluentDialog::SettingsLayout::Fill;
			}
			int controlsWidth = fixed + (sideCount > 1 ? gap * (sideCount - 1) : 0);
			int textWidth = width - pad * 2 - controlsWidth - (sideCount > 0 ? Scale(page, 24) : 0);
			int fillWidth = 0;
			if (hasFill)
			{
				fillWidth = max(Scale(page, 160), textWidth / 2); // a field shares the row with its name and description
				textWidth -= fillWidth + gap;
			}
			textWidth = max(textWidth, Scale(page, 80));

			int descriptionHeight = 0;
			if (entry.description.IsEmpty() == FALSE)
			{
				RECT measure = { 0, 0, textWidth, 0 };
				DrawText(dc, entry.description, -1, &measure, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
				descriptionHeight = measure.bottom;
			}
			int titleHeight = Scale(page, 20);
			int textBlock = titleHeight + (descriptionHeight > 0 ? Scale(page, 2) + descriptionHeight : 0);
			int band = max(Scale(page, 52), textBlock + Scale(page, 24));
			RECT row = { 0, y, width, y + band + (belowHeight > 0 ? belowHeight + Scale(page, 4) : 0) };

			// the controls, right aligned and centred on the row's band
			int right = width - pad;
			for (int c = (int)entry.controls.size() - 1; c >= 0; c--)
			{
				const auto& item = entry.controls[c];
				if (item.width == FluentDialog::SettingsLayout::Below)
				{
					continue;
				}
				HWND control = GetDlgItem(page, item.id);
				if (control == NULL)
				{
					continue;
				}
				int w = item.width == FluentDialog::SettingsLayout::Fill ? fillWidth : widths[c];
				int h = RowControlHeight(page, control, state->designed[item.id]);
				RECT r = { right - w, row.top + (band - h) / 2, right, row.top + (band - h) / 2 + h };
				state->placements.push_back({ control, r, IsClass(control, WC_COMBOBOX) ? Scale(page, 320) : h });
				right = r.left - gap;
			}

			// the name never runs under the controls, whatever their widths add up to
			int textRight = min(pad + textWidth, right + gap - Scale(page, 12));
			int textTop = row.top + (band - textBlock) / 2;
			state->placements.push_back({ state->titles[i], { pad, textTop, max(pad, textRight), textTop + titleHeight }, titleHeight });
			if (state->descriptions[i] != NULL)
			{
				int top = textTop + titleHeight + Scale(page, 2);
				state->placements.push_back({ state->descriptions[i], { pad, top, max(pad, textRight), top + descriptionHeight }, descriptionHeight });
			}

			// a list or other large control spans the row under its title
			int below = row.top + band - Scale(page, 6);
			for (const auto& item : entry.controls)
			{
				HWND control = GetDlgItem(page, item.id);
				if (item.width != FluentDialog::SettingsLayout::Below || control == NULL)
				{
					continue;
				}
				SIZE designed = state->designed[item.id];
				state->placements.push_back({ control, { pad, below, width - pad, below + designed.cy }, designed.cy });
				below += designed.cy + gap;
			}

			state->rows.push_back(row);
			y = row.bottom + Scale(page, 4);
		}

		SelectObject(dc, oldFont);
		ReleaseDC(page, dc);
		return y;
	}

	void Place(HWND page, LayoutState* state)
	{
		HDWP defer = BeginDeferWindowPos((int)state->placements.size());
		for (const Placement& p : state->placements)
		{
			if (defer != NULL)
			{
				defer = DeferWindowPos(defer, p.hwnd, NULL, p.rect.left, p.rect.top - state->scroll, p.rect.right - p.rect.left, p.height,
					SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}
		if (defer != NULL)
		{
			EndDeferWindowPos(defer);
		}
		RedrawWindow(page, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
	}

	void LayoutPage(HWND page)
	{
		LayoutState* state = StateOf(page);
		if (state == NULL || state->laying)
		{
			return;
		}
		state->laying = true;

		// the page moved to a monitor with another scale: titles and descriptions get fonts made for it
		UINT dpi = GetDpiForWindow(page);
		if (dpi != state->fontDpi)
		{
			for (HFONT font : { state->titleFont, state->descriptionFont, state->sectionFont })
			{
				if (font != NULL) DeleteObject(font);
			}
			state->titleFont = MakeFont(page, 14, FW_NORMAL);
			state->descriptionFont = MakeFont(page, 12, FW_NORMAL);
			state->sectionFont = MakeFont(page, 14, FW_SEMIBOLD);
			state->fontDpi = dpi;
			for (size_t i = 0; i < state->entries.size(); i++)
			{
				SendMessage(state->titles[i], WM_SETFONT, (WPARAM)(state->entries[i].section ? state->sectionFont : state->titleFont), FALSE);
				if (state->descriptions[i] != NULL)
				{
					SendMessage(state->descriptions[i], WM_SETFONT, (WPARAM)state->descriptionFont, FALSE);
				}
			}
		}

		RECT client;
		GetClientRect(page, &client);
		int width = client.right - (GetWindowLong(page, GWL_STYLE) & WS_VSCROLL ? 0 : Scale(page, 4));
		state->content = Measure(page, state, width);

		// Windows 11 pages scroll when they hold more than fits; the bar takes width, so measure again with it
		bool needScroll = state->content > client.bottom;
		bool hasScroll = (GetWindowLong(page, GWL_STYLE) & WS_VSCROLL) != 0;
		if (needScroll != hasScroll)
		{
			ShowScrollBar(page, SB_VERT, needScroll);
			GetClientRect(page, &client);
			state->content = Measure(page, state, client.right - (needScroll ? 0 : Scale(page, 4)));
		}

		int maxScroll = max(0, state->content - (int)client.bottom);
		state->scroll = min(max(state->scroll, 0), maxScroll);
		SCROLLINFO info = { sizeof(info), SIF_RANGE | SIF_PAGE | SIF_POS, 0, max(0, state->content - 1), (UINT)client.bottom, state->scroll };
		SetScrollInfo(page, SB_VERT, &info, TRUE);

		Place(page, state);
		state->laying = false;
	}

	void ScrollPage(HWND page, int position)
	{
		LayoutState* state = StateOf(page);
		if (state == NULL)
		{
			return;
		}
		RECT client;
		GetClientRect(page, &client);
		position = min(max(position, 0), max(0, state->content - (int)client.bottom));
		if (position == state->scroll)
		{
			return;
		}
		int delta = state->scroll - position;
		state->scroll = position;
		SetScrollPos(page, SB_VERT, position, TRUE);

		// move the pixels and the controls in one go and paint only the strip that comes into view:
		// placing and repainting every control on each wheel notch made the whole page flash
		ScrollWindowEx(page, 0, delta, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
		UpdateWindow(page);
	}

	// brings the row a control sits in fully into view (a control reached with Tab may be scrolled away)
	void ScrollIntoView(HWND page, HWND control)
	{
		LayoutState* state = StateOf(page);
		if (state == NULL)
		{
			return;
		}
		RECT client;
		GetClientRect(page, &client);
		for (const Placement& p : state->placements)
		{
			if (p.hwnd != control)
			{
				continue;
			}
			for (const RECT& row : state->rows)
			{
				if (p.rect.top >= row.top && p.rect.top < row.bottom)
				{
					int margin = Scale(page, 8);
					if (row.top - state->scroll < 0)
					{
						ScrollPage(page, row.top - margin);
					}
					else if (row.bottom - state->scroll > client.bottom)
					{
						ScrollPage(page, row.bottom - client.bottom + margin);
					}
					return;
				}
			}
		}
	}

	// every control in a settings row: focus scrolls its row into view; over a closed drop-down the wheel scrolls the
	// page, as in Windows 11 Settings, instead of changing the choice
	const UINT_PTR RowControlSubclassId = 0xF1F1;
	LRESULT CALLBACK RowControlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		switch (msg)
		{
		case WM_SETFOCUS:
			ScrollIntoView(GetParent(hwnd), hwnd);
			break;
		case WM_MOUSEWHEEL:
			if (IsClass(hwnd, WC_COMBOBOX) && SendMessage(hwnd, CB_GETDROPPEDSTATE, 0, 0) == FALSE)
			{
				return SendMessage(GetParent(hwnd), WM_MOUSEWHEEL, wParam, lParam);
			}
			break;
		case WM_NCDESTROY:
			RemoveWindowSubclass(hwnd, RowControlProc, RowControlSubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	int PageScroll(HWND page)
	{
		LayoutState* state = StateOf(page);
		return state != NULL ? state->scroll : 0;
	}

	int PageScrollMax(HWND page)
	{
		LayoutState* state = StateOf(page);
		RECT client;
		GetClientRect(page, &client);
		return state != NULL ? max(0, state->content - (int)client.bottom) : 0;
	}

	void PaintRows(HWND page, HDC target)
	{
		LayoutState* state = StateOf(page);
		RECT client;
		GetClientRect(page, &client);

		// drawn off screen and copied once, so a strip scrolled into view never shows the ground before its card
		HDC dc = CreateCompatibleDC(target);
		HBITMAP bitmap = CreateCompatibleBitmap(target, max(1, (int)client.right), max(1, (int)client.bottom));
		HGDIOBJ oldBitmap = SelectObject(dc, bitmap);

		FillRect(dc, &client, Brush(Colors().ground));
		COLORREF card = ColorsOn(page).ground;
		for (const RECT& row : state->rows)
		{
			RECT r = row;
			OffsetRect(&r, 0, -state->scroll);
			if (r.bottom > 0 && r.top < client.bottom)
			{
				FluentDialog::FillRounded(dc, r, Scale(page, 6), card);
			}
		}

		BitBlt(target, 0, 0, client.right, client.bottom, dc, 0, 0, SRCCOPY);
		SelectObject(dc, oldBitmap);
		DeleteObject(bitmap);
		DeleteDC(dc);
	}

	bool InRow(HWND page, HWND control)
	{
		LayoutState* state = StateOf(page);
		if (state == NULL)
		{
			return false;
		}
		RECT r;
		GetWindowRect(control, &r);
		MapWindowPoints(NULL, page, (POINT*)&r, 2);
		POINT center = { (r.left + r.right) / 2, (r.top + r.bottom) / 2 + state->scroll };
		for (const RECT& row : state->rows)
		{
			if (PtInRect(&row, center))
			{
				return true;
			}
		}
		return false;
	}

	void FreeLayout(HWND page)
	{
		LayoutState* state = StateOf(page);
		if (state == NULL)
		{
			return;
		}
		RemoveProp(page, LayoutProp);
		for (HWND d : state->descriptions)
		{
			if (d != NULL) RemoveProp(d, DimProp);
		}
		DeleteObject(state->titleFont);
		DeleteObject(state->descriptionFont);
		DeleteObject(state->sectionFont);
		delete state;
	}

	// "Send Paste (Otherwise it will just load the item on the clipboard)" -> title and description
	void SplitTitle(CString& title, CString& description)
	{
		int open = title.Find(_T(" ("));
		if (open > 0 && title.Right(1) == _T(")") && description.IsEmpty())
		{
			CString inside = title.Mid(open + 2, title.GetLength() - open - 3);
			if (inside.SpanIncluding(_T("0123456789")) != inside)
			{
				description = inside.Left(1).MakeUpper() + inside.Mid(1);
				title = title.Left(open);
			}
		}
	}
}

using namespace FluentInternal;

FluentDialog::SettingsLayout& FluentDialog::SettingsLayout::Section(int groupBoxId)
{
	Entry entry;
	entry.section = true;
	entry.titleId = groupBoxId;
	m_entries.push_back(entry);
	return *this;
}

FluentDialog::SettingsLayout& FluentDialog::SettingsLayout::SectionText(const CString& title)
{
	Entry entry;
	entry.section = true;
	entry.title = title;
	m_entries.push_back(entry);
	return *this;
}

FluentDialog::SettingsLayout& FluentDialog::SettingsLayout::Row(int titleId, const std::vector<Item>& controls, int descriptionId)
{
	Entry entry;
	entry.titleId = titleId;
	entry.descriptionId = descriptionId;
	entry.controls = controls;
	m_entries.push_back(entry);
	return *this;
}

FluentDialog::SettingsLayout& FluentDialog::SettingsLayout::RowText(const CString& title, const std::vector<Item>& controls, const CString& description)
{
	Entry entry;
	entry.title = title;
	entry.description = description;
	entry.controls = controls;
	m_entries.push_back(entry);
	return *this;
}

FluentDialog::SettingsLayout& FluentDialog::SettingsLayout::Describe(const CString& description)
{
	if (m_entries.empty() == false)
	{
		m_entries.back().description = description;
	}
	return *this;
}

FluentDialog::SettingsLayout& FluentDialog::SettingsLayout::Hide(const std::vector<int>& ids)
{
	m_hidden.insert(m_hidden.end(), ids.begin(), ids.end());
	return *this;
}

void FluentDialog::SettingsLayout::AttachTo(HWND page)
{
	FreeLayout(page);
	LayoutState* state = new LayoutState;
	state->entries = m_entries;
	state->titleFont = MakeFont(page, 14, FW_NORMAL);
	state->descriptionFont = MakeFont(page, 12, FW_NORMAL);
	state->sectionFont = MakeFont(page, 14, FW_SEMIBOLD);
	state->fontDpi = GetDpiForWindow(page);

	for (Entry& entry : state->entries)
	{
		// the title and description come from the page's own (translated) controls
		HWND titleControl = entry.titleId != 0 ? GetDlgItem(page, entry.titleId) : NULL;
		if (titleControl != NULL)
		{
			int length = GetWindowTextLength(titleControl);
			GetWindowText(titleControl, entry.title.GetBuffer(length + 1), length + 1);
			entry.title.ReleaseBuffer();
			entry.title.Remove('&');

			bool isControl = false;
			for (const Item& item : entry.controls)
			{
				isControl = isControl || item.id == entry.titleId;
			}
			if (isControl)
			{
				SetProp(titleControl, NoLabelProp, (HANDLE)1); // the check box shows as a bare switch; its name shows on the row
			}
			else
			{
				ShowWindow(titleControl, SW_HIDE);
			}
		}
		if (entry.descriptionId != 0)
		{
			if (HWND description = GetDlgItem(page, entry.descriptionId))
			{
				int length = GetWindowTextLength(description);
				GetWindowText(description, entry.description.GetBuffer(length + 1), length + 1);
				entry.description.ReleaseBuffer();
				ShowWindow(description, SW_HIDE);
			}
		}
		if (entry.section == false)
		{
			SplitTitle(entry.title, entry.description);
		}

		for (const Item& item : entry.controls)
		{
			if (HWND control = GetDlgItem(page, item.id))
			{
				RECT r;
				GetWindowRect(control, &r);
				state->designed[item.id] = { r.right - r.left, r.bottom - r.top };
				ShowWindow(control, SW_SHOW);
				SetWindowSubclass(control, RowControlProc, RowControlSubclassId, 0);
			}
		}

		state->titles.push_back(MakeLabel(page, entry.title, entry.section ? state->sectionFont : state->titleFont, false));
		HWND description = NULL;
		if (entry.section == false && entry.description.IsEmpty() == FALSE)
		{
			description = MakeLabel(page, entry.description, state->descriptionFont, true);
			SetProp(description, DimProp, (HANDLE)1);
		}
		state->descriptions.push_back(description);
	}

	// the old frames go: the rows and headings replace them
	EnumChildWindows(page, [](HWND child, LPARAM) -> BOOL
	{
		if (IsClass(child, _T("Button")) && (GetWindowLong(child, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX)
		{
			ShowWindow(child, SW_HIDE);
		}
		return TRUE;
	}, 0);
	for (int id : m_hidden)
	{
		if (HWND hidden = GetDlgItem(page, id))
		{
			ShowWindow(hidden, SW_HIDE);
		}
	}

	// Tab walks the rows top to bottom, left to right: the dialog's tab order is its window order
	HWND previous = HWND_TOP;
	for (const Entry& entry : state->entries)
	{
		for (const Item& item : entry.controls)
		{
			if (HWND control = GetDlgItem(page, item.id))
			{
				SetWindowPos(control, previous, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				previous = control;
			}
		}
	}

	SetProp(page, LayoutProp, (HANDLE)state);
	SetWindowSubclass(page, DialogProc, DialogSubclassId, 0);
	// the row cards are painted around the controls, never under them: no flash behind each control
	SetWindowLong(page, GWL_STYLE, GetWindowLong(page, GWL_STYLE) | WS_CLIPCHILDREN);
	if (Colors().dark)
	{
		SetWindowTheme(page, L"DarkMode_Explorer", NULL); // the page's own scroll bar
	}
	LayoutPage(page);
}

bool FluentDialog::HasSettingsLayout(HWND page)
{
	return page != NULL && GetProp(page, LayoutProp) != NULL;
}
