#include "stdafx.h"
#include "CP_Main.h"
#include "FluentDialogInternal.h"
#include <commctrl.h>
#include <dwmapi.h>
#include <Uxtheme.h>
#include <vssym32.h>
#include <Richedit.h>
#include <map>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

// the helpers FluentSettings.cpp shares are declared in FluentDialogInternal.h
namespace FluentInternal
{
	const UINT_PTR ButtonSubclassId = 0xF1E9;
	const UINT_PTR TabSubclassId = 0xF1EA;
	const UINT_PTR HotkeySubclassId = 0xF1EB;
	const UINT_PTR FrameSubclassId = 0xF1EC;
	const UINT_PTR ToolbarSubclassId = 0xF1ED;
	// window property names as atoms registered once: a property left on a control then costs nothing when it goes
	const LPCTSTR ThemedProp = MAKEINTATOM(GlobalAddAtom(_T("BulbFluentThemed")));
	const LPCTSTR HotProp = MAKEINTATOM(GlobalAddAtom(_T("BulbFluentHot")));
	const LPCTSTR PageRectProp = MAKEINTATOM(GlobalAddAtom(_T("BulbFluentPageRect")));
	const LPCTSTR CardProp = MAKEINTATOM(GlobalAddAtom(_T("BulbFluentCard"))); // value: footer top + 1, or 1 for no footer
	const LPCTSTR RowsProp = MAKEINTATOM(GlobalAddAtom(_T("BulbFluentRows"))); // a report list whose rows Bulb draws
	const LPCTSTR LinkProp = MAKEINTATOM(GlobalAddAtom(_T("BulbFluentLink")));


	Palette Colors()
	{
		COLORREF bg = CGetSetOptions::m_Theme.MainWindowBG();
		Palette c;
		c.dark = Luma(bg) < 128;
		c.ground = c.dark ? RGB(32, 32, 32) : RGB(243, 243, 243);
		c.field = c.dark ? RGB(45, 45, 45) : RGB(255, 255, 255);
		c.ink = c.dark ? RGB(255, 255, 255) : RGB(26, 26, 26);
		c.dim = c.dark ? RGB(150, 150, 150) : RGB(110, 110, 110);
		c.line = c.dark ? RGB(64, 64, 64) : RGB(220, 220, 220);
		c.selected = c.dark ? RGB(50, 50, 50) : RGB(234, 234, 234);
		c.accent = CGetSetOptions::m_Theme.SearchTextBoxFocusBorder();
		c.disabled = c.dark ? RGB(110, 110, 110) : RGB(165, 165, 165);
		c.menu = c.dark ? RGB(44, 44, 44) : RGB(249, 249, 249);
		c.menuHover = c.dark ? RGB(61, 61, 61) : RGB(234, 234, 234);
		c.navHover = c.dark ? RGB(45, 45, 45) : RGB(234, 234, 234);
		c.navSelected = c.dark ? RGB(52, 52, 52) : RGB(228, 228, 228);
		return c;
	}

	// a page of a settings-style sheet: its controls sit on a raised card
	bool IsSettingsPage(HWND hwnd)
	{
		HWND parent = GetParent(hwnd);
		return parent != NULL && (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD) && GetProp(parent, PageRectProp) != NULL;
	}

	// the colors for what is drawn on 'surface' (the window whose background shows around it)
	Palette ColorsOn(HWND surface)
	{
		Palette c = Colors();
		if (surface != NULL && (IsSettingsPage(surface) || GetProp(surface, CardProp) != NULL))
		{
			// Windows 11 Settings: cards one step lighter than the window, fields one step lighter again
			c.ground = c.dark ? RGB(43, 43, 43) : RGB(251, 251, 251);
			c.field = c.dark ? RGB(52, 52, 52) : RGB(255, 255, 255);
			c.selected = c.dark ? RGB(58, 58, 58) : RGB(238, 238, 238);
		}
		return c;
	}

	HBRUSH Brush(COLORREF color)
	{
		static std::map<COLORREF, HBRUSH> brushes;
		auto found = brushes.find(color);
		if (found != brushes.end())
		{
			return found->second;
		}
		HBRUSH brush = CreateSolidBrush(color);
		brushes[color] = brush;
		return brush;
	}

	// same pixel height as the dialog's own font, so the dialog layout does not move
	HFONT FontLike(HFONT original, bool semibold = false)
	{
		LOGFONT lf = { 0 };
		if (original == NULL || GetObject(original, sizeof(lf), &lf) == 0)
		{
			GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
		}

		static std::map<std::pair<LONG, int>, HFONT> fonts;
		auto key = std::make_pair(lf.lfHeight, semibold ? FW_SEMIBOLD : lf.lfWeight);
		auto found = fonts.find(key);
		if (found != fonts.end())
		{
			return found->second;
		}

		lf.lfWeight = key.second;
		lf.lfQuality = CLEARTYPE_QUALITY;
		lstrcpyn(lf.lfFaceName, ModernUIFontName(), LF_FACESIZE);
		HFONT font = CreateFontIndirect(&lf);
		fonts[key] = font;
		return font;
	}

	int Scale(HWND hwnd, int value)
	{
		UINT dpi = GetDpiForWindow(hwnd);
		return MulDiv(value, dpi ? dpi : 96, 96);
	}

	void RoundFrame(HDC dc, RECT r, int radius, COLORREF fill, COLORREF border, bool filled)
	{
		HPEN pen = CreatePen(PS_SOLID, 1, border);
		HGDIOBJ oldPen = SelectObject(dc, pen);
		HGDIOBJ oldBrush = SelectObject(dc, filled ? Brush(fill) : GetStockObject(NULL_BRUSH));
		RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
		SelectObject(dc, oldBrush);
		SelectObject(dc, oldPen);
		DeleteObject(pen);
	}

	bool IsLight(COLORREF color)
	{
		return Luma(color) >= 150;
	}

	CString WindowText(HWND hwnd)
	{
		CString text;
		int length = GetWindowTextLength(hwnd);
		GetWindowText(hwnd, text.GetBuffer(length + 1), length + 1);
		text.ReleaseBuffer();
		return text;
	}

	void ApplyTree(HWND root);
	HFONT IconFont(HWND hwnd, int pixels = 16);
	bool ListViewDraw(NMLVCUSTOMDRAW* draw, LRESULT& result);

	// colors for the controls a window holds; false when msg is not a WM_CTLCOLOR* message
	bool CtlColor(HWND surface, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& result)
	{
		Palette c = ColorsOn(surface);
		switch (msg)
		{
		case WM_CTLCOLORDLG:
			result = (LRESULT)Brush(c.ground);
			return true;
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORBTN:
		{
			HDC dc = (HDC)wParam;
			HWND control = (HWND)lParam;
			TCHAR cls[32] = { 0 };
			GetClassName(control, cls, 32);
			bool field = _tcsicmp(cls, _T("Edit")) == 0; // read-only edits ask with WM_CTLCOLORSTATIC
			COLORREF back = field ? c.field : c.ground;

			// a dialog's footer band keeps the window ground under its buttons
			INT_PTR footer = surface != NULL ? (INT_PTR)GetProp(surface, CardProp) - 1 : 0;
			if (footer > 0 && field == false)
			{
				RECT r;
				GetWindowRect(control, &r);
				MapWindowPoints(NULL, surface, (POINT*)&r, 2);
				if (r.top >= footer)
				{
					back = Colors().ground;
				}
			}

			// a settings page is the window ground with card rows: what sits outside a row (a heading) is on the ground
			if (surface != NULL && GetProp(surface, LayoutProp) != NULL && field == false && InRow(surface, control) == false)
			{
				back = Colors().ground;
			}

			SetTextColor(dc, IsWindowEnabled(control) == FALSE || GetProp(control, DimProp) != NULL ? c.dim :
				(GetProp(control, LinkProp) != NULL ? c.accent : c.ink));
			SetBkColor(dc, back);
			result = (LRESULT)Brush(back);
			return true;
		}
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
			SetTextColor((HDC)wParam, c.ink);
			SetBkColor((HDC)wParam, c.field);
			result = (LRESULT)Brush(c.field);
			return true;
		}
		return false;
	}

	LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		Palette c = ColorsOn(hwnd);
		LRESULT colors = 0;
		if (CtlColor(hwnd, msg, wParam, lParam, colors))
		{
			return colors;
		}

		switch (msg)
		{
		case WM_WINDOWPOSCHANGING:
			if (IsSettingsPage(hwnd))
			{
				// the property sheet places every page where its tabs used to leave room; the settings layout moved that place
				const RECT* page = (const RECT*)GetProp(GetParent(hwnd), PageRectProp);
				WINDOWPOS* pos = (WINDOWPOS*)lParam;
				pos->x = page->left;
				pos->y = page->top;
				pos->cx = page->right - page->left;
				pos->cy = page->bottom - page->top;
				pos->flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
			}
			break;
		case WM_ERASEBKGND:
		{
			if (GetProp(hwnd, LayoutProp) != NULL)
			{
				PaintRows(hwnd, (HDC)wParam);
				return TRUE;
			}
			RECT r;
			GetClientRect(hwnd, &r);
			FillRect((HDC)wParam, &r, Brush(c.ground));
			return TRUE;
		}
		case WM_SIZE:
			if (GetProp(hwnd, LayoutProp) != NULL)
			{
				LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
				LayoutPage(hwnd);
				return result;
			}
			break;
		case WM_VSCROLL:
			if (GetProp(hwnd, LayoutProp) != NULL)
			{
				RECT r;
				GetClientRect(hwnd, &r);
				int position = PageScroll(hwnd);
				int line = Scale(hwnd, 48);
				switch (LOWORD(wParam))
				{
				case SB_LINEUP: position -= line; break;
				case SB_LINEDOWN: position += line; break;
				case SB_PAGEUP: position -= r.bottom; break;
				case SB_PAGEDOWN: position += r.bottom; break;
				case SB_TOP: position = 0; break;
				case SB_BOTTOM: position = PageScrollMax(hwnd); break;
				case SB_THUMBTRACK:
				case SB_THUMBPOSITION:
				{
					SCROLLINFO info = { sizeof(info), SIF_TRACKPOS };
					GetScrollInfo(hwnd, SB_VERT, &info);
					position = info.nTrackPos;
					break;
				}
				}
				ScrollPage(hwnd, position);
				return 0;
			}
			break;
		case WM_MOUSEWHEEL:
			if (GetProp(hwnd, LayoutProp) != NULL && PageScrollMax(hwnd) > 0)
			{
				ScrollPage(hwnd, PageScroll(hwnd) - GET_WHEEL_DELTA_WPARAM(wParam) * Scale(hwnd, 48) / WHEEL_DELTA);
				return 0;
			}
			break;
		case WM_PAINT:
			// a message box paints its button band in light grey here; its text and icon are child controls
			if ((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE) == GetModuleHandle(_T("user32.dll")))
			{
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(hwnd, &ps);
				FillRect(dc, &ps.rcPaint, Brush(c.ground));
				EndPaint(hwnd, &ps);
				return 0;
			}
			break;
		case WM_NOTIFY:
		{
			LRESULT drawn = 0;
			if (((NMHDR*)lParam)->code == NM_CUSTOMDRAW && ListViewDraw((NMLVCUSTOMDRAW*)lParam, drawn))
			{
				return drawn;
			}
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			if (((NMHDR*)lParam)->code == TCN_SELCHANGE)
			{
				ApplyTree(hwnd); // the newly shown page may have just been created
			}
			return result;
		}
		case PSM_SETCURSEL:
		case PSM_SETCURSELID:
		{
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			ApplyTree(hwnd);
			return result;
		}
		case WM_NCDESTROY:
			RemoveProp(hwnd, CardProp);
			FreeLayout(hwnd);
			RemoveWindowSubclass(hwnd, DialogProc, DialogSubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	// what the parent paints behind a control (its ground, a card, a footer band), so rounded corners blend in
	HBRUSH BackgroundOf(HWND control, HDC dc, COLORREF fallback)
	{
		HBRUSH brush = (HBRUSH)SendMessage(GetParent(control), WM_CTLCOLORBTN, (WPARAM)dc, (LPARAM)control);
		return brush != NULL ? brush : Brush(fallback);
	}

	bool IsPushButton(LONG style)
	{
		LONG type = style & BS_TYPEMASK;
		return (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) && (style & (BS_ICON | BS_BITMAP)) == 0;
	}

	// Windows 11 button: rounded, quiet surface; the default one is filled with the accent
	void PaintPushButton(HWND hwnd, HDC dc)
	{
		Palette c = ColorsOn(GetParent(hwnd));
		RECT r;
		GetClientRect(hwnd, &r);
		FillRect(dc, &r, BackgroundOf(hwnd, dc, c.ground));

		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		bool isDefault = (style & BS_TYPEMASK) == BS_DEFPUSHBUTTON;
		bool enabled = IsWindowEnabled(hwnd) != FALSE;
		bool pressed = (SendMessage(hwnd, BM_GETSTATE, 0, 0) & BST_PUSHED) != 0;
		bool hot = GetProp(hwnd, HotProp) != NULL;

		COLORREF fill = isDefault ? c.accent : c.field;
		COLORREF border = isDefault ? c.accent : c.line;
		COLORREF text = isDefault ? (IsLight(c.accent) ? RGB(0, 0, 0) : RGB(255, 255, 255)) : c.ink;
		if (enabled == false)
		{
			fill = c.selected;
			border = c.line;
			text = c.dim;
		}
		else if (pressed)
		{
			fill = Mix(fill, c.ground, 30);
		}
		else if (hot)
		{
			fill = Mix(fill, c.ink, 7);
		}

		RECT body = r;
		InflateRect(&body, -1, -1);
		RoundFrame(dc, body, Scale(hwnd, 8), fill, border, true);

		LRESULT uiState = SendMessage(hwnd, WM_QUERYUISTATE, 0, 0);
		if (GetFocus() == hwnd && (uiState & UISF_HIDEFOCUS) == 0)
		{
			RECT ring = body;
			InflateRect(&ring, -2, -2);
			RoundFrame(dc, ring, Scale(hwnd, 6), fill, isDefault ? text : c.accent, false);
		}

		HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
		SetBkMode(dc, TRANSPARENT);
		SetTextColor(dc, text);
		UINT format = DT_CENTER | DT_VCENTER | DT_SINGLELINE | ((uiState & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0);
		if (style & BS_MULTILINE)
		{
			format = DT_CENTER | DT_WORDBREAK | ((uiState & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0);
		}
		CString label = WindowText(hwnd);
		if (format & DT_WORDBREAK)
		{
			RECT measure = body;
			DrawText(dc, label, -1, &measure, format | DT_CALCRECT);
			body.top += max(0, ((body.bottom - body.top) - (measure.bottom - measure.top)) / 2);
		}
		DrawText(dc, label, -1, &body, format);
		SelectObject(dc, oldFont);
	}

	Gdiplus::Color Gdi(COLORREF color)
	{
		return Gdiplus::Color(255, GetRValue(color), GetGValue(color), GetBValue(color));
	}

	// Windows 11 switch: an outlined pill with a small knob on the left when off,
	// an accent pill with the knob on the right when on
	void PaintSwitch(HDC dc, const RECT& box, bool on, bool enabled, bool hot, const Palette& c)
	{
		Gdiplus::Graphics g(dc);
		g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		Gdiplus::REAL left = (Gdiplus::REAL)box.left + 0.5f, top = (Gdiplus::REAL)box.top + 0.5f;
		Gdiplus::REAL w = (Gdiplus::REAL)(box.right - box.left) - 1, h = (Gdiplus::REAL)(box.bottom - box.top) - 1;

		Gdiplus::GraphicsPath pill;
		pill.AddArc(left, top, h, h, 90, 180);
		pill.AddArc(left + w - h, top, h, h, 270, 180);
		pill.CloseFigure();

		Gdiplus::REAL knob = h * (hot && enabled ? 0.62f : 0.52f);
		Gdiplus::REAL knobTop = top + (h - knob) / 2;
		if (on)
		{
			COLORREF fill = enabled ? (hot ? Mix(c.accent, c.ink, 10) : c.accent) : c.dim;
			Gdiplus::SolidBrush pillBrush(Gdi(fill));
			g.FillPath(&pillBrush, &pill);
			Gdiplus::SolidBrush knobBrush(Gdi(IsLight(fill) ? RGB(0, 0, 0) : RGB(255, 255, 255)));
			g.FillEllipse(&knobBrush, left + w - (h - knob) / 2 - knob, knobTop, knob, knob);
		}
		else
		{
			if (hot && enabled)
			{
				Gdiplus::SolidBrush hover(Gdi(Mix(c.ground, c.ink, 7)));
				g.FillPath(&hover, &pill);
			}
			COLORREF mark = enabled ? (hot ? c.ink : Mix(c.dim, c.ink, 30)) : c.line;
			Gdiplus::Pen outline(Gdi(mark), 1.0f);
			g.DrawPath(&outline, &pill);
			Gdiplus::SolidBrush knobBrush(Gdi(mark));
			g.FillEllipse(&knobBrush, left + (h - knob) / 2, knobTop, knob, knob);
		}
	}

	void PaintButton(HWND hwnd, HDC dc)
	{
		Palette c = ColorsOn(GetParent(hwnd));
		RECT r;
		GetClientRect(hwnd, &r);

		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		if (IsPushButton(style))
		{
			PaintPushButton(hwnd, dc);
			return;
		}
		LONG type = style & BS_TYPEMASK;
		// a group box sits behind the controls it frames: paint only its outline, never its inside
		if (type != BS_GROUPBOX)
		{
			FillRect(dc, &r, BackgroundOf(hwnd, dc, c.ground));
		}
		bool enabled = IsWindowEnabled(hwnd) != FALSE;
		LRESULT uiState = SendMessage(hwnd, WM_QUERYUISTATE, 0, 0);

		// a switch in a settings row keeps its text (screen readers and access keys use it) but the row shows it
		CString text;
		if (GetProp(hwnd, NoLabelProp) == NULL)
		{
			int length = GetWindowTextLength(hwnd);
			GetWindowText(hwnd, text.GetBuffer(length + 1), length + 1);
			text.ReleaseBuffer();
		}

		HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
		SetBkMode(dc, TRANSPARENT);
		SetTextColor(dc, enabled ? c.ink : c.dim);
		UINT prefix = (uiState & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0;

		if (type == BS_GROUPBOX)
		{
			// a quiet rounded frame with its title cut into the top line
			SIZE size = { 0 };
			GetTextExtentPoint32(dc, text, text.GetLength(), &size);
			RECT frame = { r.left, r.top + size.cy / 2, r.right, r.bottom };
			RoundFrame(dc, frame, Scale(hwnd, 8), c.ground, c.line, false);
			if (text.IsEmpty() == FALSE)
			{
				RECT title = { r.left + Scale(hwnd, 8), r.top, r.left + Scale(hwnd, 16) + size.cx, r.top + size.cy };
				FillRect(dc, &title, Brush(c.ground));
				title.left += Scale(hwnd, 4);
				DrawText(dc, text, -1, &title, DT_LEFT | DT_TOP | DT_SINGLELINE | prefix);
			}
		}
		else
		{
			bool radio = type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON;
			LRESULT check = SendMessage(hwnd, BM_GETCHECK, 0, 0);
			LRESULT state = SendMessage(hwnd, BM_GETSTATE, 0, 0);

			// on a settings page an on/off check box is a switch, as in Windows 11 Settings, when its label still fits;
			// it is never taller than the box it lives in, or its round ends would be cut
			int switchHeight = min(Scale(hwnd, 18), (int)(r.bottom - r.top) - 2);
			SIZE switchSize = { switchHeight * 34 / 18, switchHeight };
			bool asSwitch = false;
			if ((type == BS_CHECKBOX || type == BS_AUTOCHECKBOX) && IsSettingsPage(GetParent(hwnd)))
			{
				SIZE label = { 0 };
				GetTextExtentPoint32(dc, text, text.GetLength(), &label);
				asSwitch = (style & BS_MULTILINE) || switchSize.cx + Scale(hwnd, 8) + label.cx <= r.right - r.left;
			}

			RECT boxRect;
			if (asSwitch)
			{
				boxRect = { r.left, (r.top + r.bottom - switchSize.cy) / 2, r.left + switchSize.cx, (r.top + r.bottom - switchSize.cy) / 2 + switchSize.cy };
				PaintSwitch(dc, boxRect, check == BST_CHECKED, enabled, GetProp(hwnd, HotProp) != NULL, c);
			}
			else
			{
				int part = radio ? BP_RADIOBUTTON : BP_CHECKBOX;
				int base = check == BST_CHECKED ? 5 : (check == BST_INDETERMINATE && radio == false ? 9 : 1);
				int partState = base + (enabled == false ? 3 : ((state & BST_PUSHED) ? 2 : 0));

				SIZE box = { Scale(hwnd, 13), Scale(hwnd, 13) };
				HTHEME theme = OpenThemeData(hwnd, L"Button");
				if (theme != NULL)
				{
					GetThemePartSize(theme, dc, part, partState, NULL, TS_DRAW, &box);
				}
				boxRect = { r.left, (r.top + r.bottom - box.cy) / 2, r.left + box.cx, (r.top + r.bottom - box.cy) / 2 + box.cy };
				if (theme != NULL)
				{
					DrawThemeBackground(theme, dc, part, partState, &boxRect, NULL);
					CloseThemeData(theme);
				}
				else
				{
					DrawFrameControl(dc, &boxRect, DFC_BUTTON, (radio ? DFCS_BUTTONRADIO : DFCS_BUTTONCHECK) | (check ? DFCS_CHECKED : 0));
				}
			}

			RECT textRect = r;
			textRect.left = boxRect.right + Scale(hwnd, asSwitch ? 8 : 6);
			UINT format = DT_LEFT | prefix | DT_END_ELLIPSIS | ((style & BS_MULTILINE) ? DT_WORDBREAK : (DT_SINGLELINE | DT_VCENTER));
			if (style & BS_MULTILINE)
			{
				RECT measure = textRect;
				DrawText(dc, text, -1, &measure, format | DT_CALCRECT);
				int offset = ((r.bottom - r.top) - (measure.bottom - measure.top)) / 2;
				textRect.top += max(0, offset);
			}
			DrawText(dc, text, -1, &textRect, format);

			if (GetFocus() == hwnd && (uiState & UISF_HIDEFOCUS) == 0)
			{
				// Windows 11 keyboard focus: a rounded ring around the whole control, not a dotted box
				RECT focus = textRect;
				DrawText(dc, text, -1, &focus, format | DT_CALCRECT);
				RECT ring = { r.left, min(boxRect.top, focus.top) - 1, min(r.right, focus.right + Scale(hwnd, 4)), max(boxRect.bottom, focus.bottom) + 1 };
				ring.top = max(r.top, ring.top);
				ring.bottom = min(r.bottom, ring.bottom);
				FluentDialog::OutlineRounded(dc, ring, Scale(hwnd, 4), c.ink);
			}
		}

		SelectObject(dc, oldFont);
	}

	LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			PaintButton(hwnd, dc);
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_PRINTCLIENT:
			PaintButton(hwnd, (HDC)wParam);
			return 0;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_MOUSEMOVE:
			if (GetProp(hwnd, HotProp) == NULL)
			{
				SetProp(hwnd, HotProp, (HANDLE)1);
				TRACKMOUSEEVENT track = { sizeof(track), TME_LEAVE, hwnd, 0 };
				TrackMouseEvent(&track);
				InvalidateRect(hwnd, NULL, FALSE);
			}
			break;
		case WM_MOUSELEAVE:
			RemoveProp(hwnd, HotProp);
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		case WM_ENABLE:
		case WM_SETTEXT:
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_UPDATEUISTATE:
		case BM_SETCHECK:
		case BM_SETSTATE:
		case BM_SETSTYLE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		{
			// the button paints some state changes straight away, in its own colors; paint over them
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			InvalidateRect(hwnd, NULL, FALSE);
			return result;
		}
		case WM_NCDESTROY:
			RemoveProp(hwnd, HotProp);
			RemoveWindowSubclass(hwnd, ButtonProc, ButtonSubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	void PaintTabs(HWND hwnd, HDC dc)
	{
		Palette c = Colors();
		RECT r;
		GetClientRect(hwnd, &r);
		FillRect(dc, &r, Brush(c.ground));

		int count = TabCtrl_GetItemCount(hwnd);
		int selected = TabCtrl_GetCurSel(hwnd);
		HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
		SetBkMode(dc, TRANSPARENT);

		for (int i = 0; i < count; i++)
		{
			RECT item;
			TabCtrl_GetItemRect(hwnd, i, &item);
			TCHAR text[128] = { 0 };
			TCITEM info = { 0 };
			info.mask = TCIF_TEXT;
			info.pszText = text;
			info.cchTextMax = 128;
			TabCtrl_GetItem(hwnd, i, &info);

			if (i == selected)
			{
				// Windows 11 navigation: a soft pill with an accent bar under the label
				RECT pill = item;
				InflateRect(&pill, -Scale(hwnd, 2), -Scale(hwnd, 2));
				RoundFrame(dc, pill, Scale(hwnd, 8), c.selected, c.selected, true);
				int barWidth = Scale(hwnd, 16);
				int center = (item.left + item.right) / 2;
				RECT bar = { center - barWidth / 2, pill.bottom - Scale(hwnd, 3), center + barWidth / 2, pill.bottom };
				RoundFrame(dc, bar, Scale(hwnd, 3), c.accent, c.accent, true);
			}

			SetTextColor(dc, i == selected ? c.ink : c.dim);
			DrawText(dc, text, -1, &item, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX);
		}

		SelectObject(dc, oldFont);
	}

	LRESULT CALLBACK TabProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			PaintTabs(hwnd, dc);
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_ERASEBKGND:
			return TRUE;
		case WM_NCDESTROY:
			RemoveWindowSubclass(hwnd, TabProc, TabSubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	CString KeyName(UINT vk, bool extended)
	{
		LONG lParam = (LONG)(MapVirtualKey(vk, MAPVK_VK_TO_VSC) << 16) | (extended ? (1 << 24) : 0);
		TCHAR name[64] = { 0 };
		return GetKeyNameText(lParam, name, 64) > 0 ? CString(name) : CString();
	}

	// the hotkey control draws itself white whatever its parent says, so it is painted here: "Ctrl + Shift + V"
	void PaintHotkey(HWND hwnd, HDC dc)
	{
		Palette c = ColorsOn(GetParent(hwnd));
		RECT r;
		GetClientRect(hwnd, &r);
		FillRect(dc, &r, Brush(c.ground));

		bool focused = GetFocus() == hwnd;
		bool enabled = IsWindowEnabled(hwnd) != FALSE;
		RECT body = r;
		RoundFrame(dc, body, Scale(hwnd, 8), enabled ? c.field : c.ground, focused ? c.accent : c.line, true);
		if (focused)
		{
			// Windows 11 marks the active field with a line along its bottom edge
			RECT bar = { body.left + Scale(hwnd, 3), body.bottom - Scale(hwnd, 2), body.right - Scale(hwnd, 3), body.bottom };
			FillRect(dc, &bar, Brush(c.accent));
		}

		WORD hotkey = (WORD)SendMessage(hwnd, HKM_GETHOTKEY, 0, 0);
		BYTE vk = LOBYTE(hotkey);
		BYTE mods = HIBYTE(hotkey);
		CString text;
		// modifiers spelled the way Windows 11 writes them everywhere; the keyboard layout would say CTRL or MAIUSC
		if (mods & HOTKEYF_CONTROL) text += _T("Ctrl + ");
		if (mods & HOTKEYF_SHIFT) text += _T("Shift + ");
		if (mods & HOTKEYF_ALT) text += _T("Alt + ");
		if (vk != 0) text += KeyName(vk, (mods & HOTKEYF_EXT) != 0);
		if (text.IsEmpty()) text = theApp.m_Language.GetString(_T("None"), _T("None"));

		HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
		SetBkMode(dc, TRANSPARENT);
		SetTextColor(dc, enabled && hotkey != 0 ? c.ink : c.dim);
		RECT textRect = body;
		textRect.left += Scale(hwnd, 8);
		textRect.right -= Scale(hwnd, 4);
		DrawText(dc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
		SelectObject(dc, oldFont);
	}

	LRESULT CALLBACK HotkeyProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			PaintHotkey(hwnd, dc);
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_ERASEBKGND:
			return TRUE;
		case WM_SETFOCUS:
		{
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			HideCaret(hwnd); // its caret would sit where the control's own text used to be
			InvalidateRect(hwnd, NULL, FALSE);
			return result;
		}
		case WM_KILLFOCUS:
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_CHAR:
		case WM_ENABLE:
		case HKM_SETHOTKEY:
		{
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			InvalidateRect(hwnd, NULL, FALSE);
			return result;
		}
		case WM_NCDESTROY:
			RemoveWindowSubclass(hwnd, HotkeyProc, HotkeySubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	int BorderRings(HWND hwnd)
	{
		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
		return ((style & WS_BORDER) ? 1 : 0) + ((exStyle & WS_EX_CLIENTEDGE) ? 2 : 0) + ((exStyle & WS_EX_STATICEDGE) ? 1 : 0);
	}

	// a one-line text field with a border: drawn rounded like the drop-downs and hotkey fields beside it
	bool IsRoundedField(HWND hwnd)
	{
		TCHAR cls[16] = { 0 };
		GetClassName(hwnd, cls, 16);
		return _tcsicmp(cls, _T("Edit")) == 0 && (GetWindowLong(hwnd, GWL_STYLE) & ES_MULTILINE) == 0 && BorderRings(hwnd) > 0;
	}

	// the ring a rounded field needs around its text so the corners clear it
	int FieldRing(HWND hwnd)
	{
		return Scale(hwnd, 2);
	}

	const UINT_PTR DateSubclassId = 0xF1F4;

	// a date or time picker draws its own square light border inside its client area: drawn again rounded and quiet
	LRESULT CALLBACK DateProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		switch (msg)
		{
		case WM_PAINT:
		{
			Palette c = ColorsOn(GetParent(hwnd));
			if (IsWindowEnabled(hwnd) == FALSE)
			{
				// switched off, the picker fills itself white even in the dark theme: draw its value dimmed instead
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(hwnd, &ps);
				RECT r;
				GetClientRect(hwnd, &r);
				HWND arrows = FindWindowEx(hwnd, NULL, UPDOWN_CLASS, NULL);
				if (arrows != NULL && IsWindowVisible(arrows))
				{
					RECT a;
					GetWindowRect(arrows, &a);
					MapWindowPoints(NULL, hwnd, (POINT*)&a, 2);
					ExcludeClipRect(dc, a.left, a.top, a.right, a.bottom);
				}
				FillRect(dc, &r, Brush(c.ground));
				RoundFrame(dc, r, Scale(hwnd, 8), c.ground, c.line, true);
				HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
				SetBkMode(dc, TRANSPARENT);
				SetTextColor(dc, c.dim);
				bool dropDown = (GetWindowLong(hwnd, GWL_STYLE) & DTS_UPDOWN) == 0;
				RECT text = { r.left + Scale(hwnd, 4), r.top, r.right - Scale(hwnd, dropDown ? 20 : 2), r.bottom };
				DrawText(dc, WindowText(hwnd), -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
				if (dropDown)
				{
					SelectObject(dc, IconFont(hwnd, 10));
					RECT arrow = { r.right - Scale(hwnd, 20), r.top, r.right - Scale(hwnd, 4), r.bottom };
					DrawText(dc, _T("\xE70D"), 1, &arrow, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				}
				SelectObject(dc, oldFont);
				EndPaint(hwnd, &ps);
				return 0;
			}
			// the picker paints into a bitmap first: in the dark theme its white body and black text become the field's
			// colors (the dark control theme leaves them light), the accent of a selected part stays
			PAINTSTRUCT ps;
			HDC target = BeginPaint(hwnd, &ps);
			RECT r;
			GetClientRect(hwnd, &r);
			int width = max(1, (int)r.right), height = max(1, (int)r.bottom);
			BITMAPINFO info = { 0 };
			info.bmiHeader = { sizeof(BITMAPINFOHEADER), width, -height, 1, 32, BI_RGB };
			void* bits = NULL;
			HDC dc = CreateCompatibleDC(target);
			HBITMAP bitmap = CreateDIBSection(target, &info, DIB_RGB_COLORS, &bits, NULL, 0);
			HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
			FillRect(dc, &r, Brush(c.field));
			DefSubclassProc(hwnd, WM_PAINT, (WPARAM)dc, 0);
			GdiFlush();
			if (c.dark && bits != NULL)
			{
				// each channel on its own, so the colored ClearType edges of the letters turn with them; only the selected
				// part's highlight keeps its color
				COLORREF highlight = GetSysColor(COLOR_HIGHLIGHT);
				auto channel = [](int value, int ink, int field) { return (ink * (255 - value) + field * value) / 255; };
				DWORD* pixel = (DWORD*)bits;
				for (int i = 0; i < width * height; i++)
				{
					int red = (pixel[i] >> 16) & 0xFF, green = (pixel[i] >> 8) & 0xFF, blue = pixel[i] & 0xFF;
					if (abs(red - GetRValue(highlight)) + abs(green - GetGValue(highlight)) + abs(blue - GetBValue(highlight)) < 60)
					{
						continue;
					}
					pixel[i] = (channel(red, GetRValue(c.ink), GetRValue(c.field)) << 16) |
						(channel(green, GetGValue(c.ink), GetGValue(c.field)) << 8) | channel(blue, GetBValue(c.ink), GetBValue(c.field));
				}
			}

			// a rounded quiet frame over the square light one
			RECT inside = r;
			InflateRect(&inside, -Scale(hwnd, 2), -Scale(hwnd, 2));
			ExcludeClipRect(dc, inside.left, inside.top, inside.right, inside.bottom);
			FillRect(dc, &r, Brush(c.ground));
			RoundFrame(dc, r, Scale(hwnd, 8), c.field, GetFocus() == hwnd ? c.accent : c.line, true);
			SelectClipRgn(dc, NULL);

			// the time picker's arrows are a child window that paints itself
			HWND arrows = FindWindowEx(hwnd, NULL, UPDOWN_CLASS, NULL);
			if (arrows != NULL && IsWindowVisible(arrows))
			{
				RECT a;
				GetWindowRect(arrows, &a);
				MapWindowPoints(NULL, hwnd, (POINT*)&a, 2);
				ExcludeClipRect(target, a.left, a.top, a.right, a.bottom);
			}
			BitBlt(target, 0, 0, width, height, dc, 0, 0, SRCCOPY);
			SelectObject(dc, oldBitmap);
			DeleteObject(bitmap);
			DeleteDC(dc);
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_ERASEBKGND:
			return TRUE;
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_ENABLE:
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case DTM_SETSYSTEMTIME:
		{
			// the picker repaints a changed part straight away in its own colors; paint over it at once
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			return result;
		}
		case WM_NCDESTROY:
			RemoveWindowSubclass(hwnd, DateProc, DateSubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	// fields and lists draw their WS_BORDER / client edge in a light system color; redraw it as a quiet line, the accent while focused
	LRESULT CALLBACK FrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		switch (msg)
		{
		case WM_NCCALCSIZE:
			if (IsRoundedField(hwnd))
			{
				LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
				int extra = FieldRing(hwnd) - BorderRings(hwnd);
				if (extra > 0)
				{
					InflateRect((RECT*)lParam, -extra, -extra); // rgrc[0] when wParam is TRUE
				}
				return result;
			}
			break;
		case WM_NCPAINT:
			if (IsRoundedField(hwnd))
			{
				Palette c = ColorsOn(GetParent(hwnd));
				HDC dc = GetWindowDC(hwnd);
				RECT r, client;
				GetWindowRect(hwnd, &r);
				GetClientRect(hwnd, &client);
				MapWindowPoints(hwnd, NULL, (POINT*)&client, 2);
				OffsetRect(&client, -r.left, -r.top);
				OffsetRect(&r, -r.left, -r.top);
				ExcludeClipRect(dc, client.left, client.top, client.right, client.bottom);

				bool focused = GetFocus() == hwnd;
				FillRect(dc, &r, Brush(c.ground));
				RoundFrame(dc, r, Scale(hwnd, 8), c.field, focused ? c.accent : c.line, true);
				if (focused)
				{
					RECT bar = { r.left + Scale(hwnd, 3), r.bottom - Scale(hwnd, 2), r.right - Scale(hwnd, 3), r.bottom };
					FillRect(dc, &bar, Brush(c.accent));
				}
				ReleaseDC(hwnd, dc);
				return 0;
			}
			else
			{
				// lists and multi-line fields keep a square frame
				LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
				Palette c = ColorsOn(GetParent(hwnd));
				int rings = BorderRings(hwnd);
				if (rings > 0)
				{
					HDC dc = GetWindowDC(hwnd);
					RECT r;
					GetWindowRect(hwnd, &r);
					OffsetRect(&r, -r.left, -r.top);
					TCHAR cls[32] = { 0 };
					GetClassName(hwnd, cls, 32);
					bool focused = GetFocus() == hwnd && _tcsicmp(cls, _T("ListBox")) != 0; // a list shows focus on its rows
					FrameRect(dc, &r, Brush(focused ? c.accent : c.line));
					for (int i = 1; i < rings; i++)
					{
						InflateRect(&r, -1, -1);
						FrameRect(dc, &r, Brush(IsWindowEnabled(hwnd) ? c.field : c.ground));
					}
					ReleaseDC(hwnd, dc);
				}
				return result;
			}
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_ENABLE:
		{
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			RedrawWindow(hwnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
			return result;
		}
		case WM_NCDESTROY:
			RemoveWindowSubclass(hwnd, FrameProc, FrameSubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	// Segoe Fluent Icons for the editor's toolbar commands; 0 = not a button we draw (a slot under a combo box)
	WCHAR GlyphForCommand(int id)
	{
		switch (id)
		{
		case ID_BUTTON_NEW: return 0xE710;                 // Add
		case ID_BUTTON_SAVE: return 0xE74E;                // Save
		case ID_BUTTON_SAVE_ALL: return 0xE78C;            // SaveLocal
		case ID_BUTTON_SAVE_CLOSE_CLIPBOARD: return 0xE8C8; // Copy
		case ID_BUTTON_CLOSE: return 0xE711;               // Cancel
		case BUTTON_FONT: return 0xE8D2;                   // Font
		case BUTTON_BOLD: return 0xE8DD;
		case BUTTON_ITALIC: return 0xE8DB;
		case BUTTON_UNDERLINE: return 0xE8DC;
		case BUTTON_LEFTALIGN: return 0xE8E4;
		case BUTTON_CENTERALIGN: return 0xE8E3;
		case BUTTON_RIGHTALIGN: return 0xE8E2;
		case BUTTON_INDENT: return 0xE291;
		case BUTTON_OUTDENT: return 0xE290;
		case BUTTON_BULLET: return 0xE8FD;
		case ID_BUTTONWRAP: return 0xE751;                 // wrap long lines
		}
		return 0;
	}

	HFONT IconFont(HWND hwnd, int pixels)
	{
		static std::map<int, HFONT> fonts;
		int size = Scale(hwnd, pixels);
		auto found = fonts.find(size);
		if (found != fonts.end())
		{
			return found->second;
		}
		HFONT font = CreateFont(-size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH, ModernIconFontName());
		fonts[size] = font;
		return font;
	}

	const UINT_PTR ComboSubclassId = 0xF1F2;

	// a drop-down list drawn as a Windows 11 field: rounded, the choice on the left and a chevron on the right,
	// in place of the system's square light frame and arrow button
	void PaintCombo(HWND hwnd, HDC dc)
	{
		Palette c = ColorsOn(GetParent(hwnd));
		RECT r;
		GetClientRect(hwnd, &r);
		FillRect(dc, &r, BackgroundOf(hwnd, dc, c.ground));

		bool enabled = IsWindowEnabled(hwnd) != FALSE;
		bool focused = GetFocus() == hwnd;
		bool hot = GetProp(hwnd, HotProp) != NULL || SendMessage(hwnd, CB_GETDROPPEDSTATE, 0, 0) != FALSE;
		COLORREF fill = enabled ? (hot ? Mix(c.field, c.ink, 5) : c.field) : c.ground;
		RoundFrame(dc, r, Scale(hwnd, 8), fill, focused ? c.accent : c.line, true);
		if (focused)
		{
			RECT bar = { r.left + Scale(hwnd, 3), r.bottom - Scale(hwnd, 2), r.right - Scale(hwnd, 3), r.bottom };
			FillRect(dc, &bar, Brush(c.accent));
		}

		CString text;
		int current = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0);
		int length = current >= 0 ? (int)SendMessage(hwnd, CB_GETLBTEXTLEN, current, 0) : CB_ERR;
		if (length > 0)
		{
			SendMessage(hwnd, CB_GETLBTEXT, current, (LPARAM)text.GetBuffer(length + 1));
			text.ReleaseBuffer();
		}

		SetBkMode(dc, TRANSPARENT);
		int arrowWidth = Scale(hwnd, 28);
		HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
		SetTextColor(dc, enabled ? c.ink : c.dim);
		RECT textRect = { r.left + Scale(hwnd, 10), r.top, r.right - arrowWidth, r.bottom };
		DrawText(dc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

		SelectObject(dc, IconFont(hwnd, 10));
		SetTextColor(dc, enabled ? Mix(c.dim, c.ink, 40) : c.dim);
		RECT arrow = { r.right - arrowWidth, r.top, r.right - Scale(hwnd, 6), r.bottom };
		DrawText(dc, _T("\xE70D"), 1, &arrow, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX); // ChevronDown
		SelectObject(dc, oldFont);
	}

	LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			PaintCombo(hwnd, dc);
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_PRINTCLIENT:
			PaintCombo(hwnd, (HDC)wParam);
			return 0;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_MOUSEMOVE:
			if (GetProp(hwnd, HotProp) == NULL)
			{
				SetProp(hwnd, HotProp, (HANDLE)1);
				TRACKMOUSEEVENT track = { sizeof(track), TME_LEAVE, hwnd, 0 };
				TrackMouseEvent(&track);
				InvalidateRect(hwnd, NULL, FALSE);
			}
			break;
		case WM_MOUSELEAVE:
			RemoveProp(hwnd, HotProp);
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		case WM_KEYDOWN:
		case WM_CHAR:
		{
			// arrow keys move the choice; while the list is open it marks the new row straight away in system colors
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			COMBOBOXINFO info = { sizeof(info) };
			if (GetComboBoxInfo(hwnd, &info) && IsWindowVisible(info.hwndList))
			{
				RedrawWindow(info.hwndList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			}
			InvalidateRect(hwnd, NULL, FALSE);
			return result;
		}
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_ENABLE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case CB_SETCURSEL:
		case CB_SELECTSTRING:
		case CB_RESETCONTENT:
		case CB_SHOWDROPDOWN:
		{
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			InvalidateRect(hwnd, NULL, FALSE);
			return result;
		}
		case WM_NCDESTROY:
			RemoveProp(hwnd, HotProp);
			RemoveWindowSubclass(hwnd, ComboProc, ComboSubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	// a toolbar of old 16-color bitmaps, drawn again as Windows 11 icon buttons
	void PaintToolbar(HWND hwnd, HDC target)
	{
		Palette c = Colors();
		RECT client;
		GetClientRect(hwnd, &client);

		HDC dc = CreateCompatibleDC(target);
		HBITMAP bitmap = CreateCompatibleBitmap(target, client.right, client.bottom);
		HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
		FillRect(dc, &client, Brush(c.ground));

		HGDIOBJ oldFont = SelectObject(dc, IconFont(hwnd));
		SetBkMode(dc, TRANSPARENT);
		int hot = (int)(INT_PTR)GetProp(hwnd, HotProp) - 1;

		int count = (int)SendMessage(hwnd, TB_BUTTONCOUNT, 0, 0);
		for (int i = 0; i < count; i++)
		{
			TBBUTTON button = { 0 };
			RECT r;
			if (SendMessage(hwnd, TB_GETBUTTON, i, (LPARAM)&button) == FALSE || SendMessage(hwnd, TB_GETITEMRECT, i, (LPARAM)&r) == FALSE ||
				(button.fsState & TBSTATE_HIDDEN))
			{
				continue;
			}

			if (button.fsStyle & TBSTYLE_SEP)
			{
				int x = (r.left + r.right) / 2;
				RECT line = { x, r.top + Scale(hwnd, 6), x + 1, r.bottom - Scale(hwnd, 6) };
				FillRect(dc, &line, Brush(c.line));
				continue;
			}

			WCHAR glyph = GlyphForCommand(button.idCommand);
			if (glyph == 0)
			{
				continue;
			}

			bool enabled = (button.fsState & TBSTATE_ENABLED) != 0;
			bool checked = (button.fsState & TBSTATE_CHECKED) != 0;
			bool pressed = (button.fsState & TBSTATE_PRESSED) != 0;
			RECT pill = r;
			InflateRect(&pill, -Scale(hwnd, 1), -Scale(hwnd, 1));
			if (enabled && (checked || pressed || i == hot))
			{
				COLORREF fill = pressed ? Mix(c.ground, c.ink, 5) : (checked ? c.selected : Mix(c.ground, c.ink, 9));
				RoundFrame(dc, pill, Scale(hwnd, 8), fill, fill, true);
			}

			SetTextColor(dc, enabled ? (checked ? c.accent : c.ink) : c.dim);
			WCHAR text[2] = { glyph, 0 };
			DrawTextW(dc, text, 1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}

		BitBlt(target, 0, 0, client.right, client.bottom, dc, 0, 0, SRCCOPY);
		SelectObject(dc, oldFont);
		SelectObject(dc, oldBitmap);
		DeleteObject(bitmap);
		DeleteDC(dc);
	}

	LRESULT CALLBACK ToolbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		LRESULT colors = 0;
		if (CtlColor(hwnd, msg, wParam, lParam, colors)) // the editor's font and size boxes sit on the toolbar
		{
			return colors;
		}

		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			PaintToolbar(hwnd, dc);
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_ERASEBKGND:
			return TRUE;
		case WM_NCPAINT:
		{
			// the toolbar's border lines are drawn in the system's light colors
			RECT window, client;
			GetWindowRect(hwnd, &window);
			GetClientRect(hwnd, &client);
			MapWindowPoints(hwnd, NULL, (POINT*)&client, 2);
			OffsetRect(&client, -window.left, -window.top);
			OffsetRect(&window, -window.left, -window.top);
			HDC dc = GetWindowDC(hwnd);
			ExcludeClipRect(dc, client.left, client.top, client.right, client.bottom);
			FillRect(dc, &window, Brush(Colors().ground));
			ReleaseDC(hwnd, dc);
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			int index = (int)SendMessage(hwnd, TB_HITTEST, 0, (LPARAM)&pt);
			int hot = (int)(INT_PTR)GetProp(hwnd, HotProp) - 1;
			if (index != hot)
			{
				if (index >= 0)
				{
					SetProp(hwnd, HotProp, (HANDLE)(INT_PTR)(index + 1));
					TRACKMOUSEEVENT track = { sizeof(track), TME_LEAVE, hwnd, 0 };
					TrackMouseEvent(&track);
				}
				else
				{
					RemoveProp(hwnd, HotProp);
				}
				InvalidateRect(hwnd, NULL, FALSE);
			}
			break;
		}
		case WM_MOUSELEAVE:
			RemoveProp(hwnd, HotProp);
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case TB_SETSTATE:
		case TB_CHECKBUTTON:
		case TB_ENABLEBUTTON:
		case TB_PRESSBUTTON:
		{
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			InvalidateRect(hwnd, NULL, FALSE);
			return result;
		}
		case WM_NCDESTROY:
			RemoveProp(hwnd, HotProp);
			RemoveWindowSubclass(hwnd, ToolbarProc, ToolbarSubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	const UINT_PTR ListSubclassId = 0xF1EF;

	// a plain list box selects in the system's saturated blue; drawn again with the navigation's soft pill and accent bar
	void PaintList(HWND hwnd, HDC target)
	{
		Palette c = ColorsOn(GetParent(hwnd));
		RECT client;
		GetClientRect(hwnd, &client);
		HDC dc = CreateCompatibleDC(target);
		HBITMAP bitmap = CreateCompatibleBitmap(target, max(1, (int)client.right), max(1, (int)client.bottom));
		HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
		FillRect(dc, &client, Brush(c.field));

		HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
		SetBkMode(dc, TRANSPARENT);
		bool multi = (GetWindowLong(hwnd, GWL_STYLE) & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != 0;
		int current = (int)SendMessage(hwnd, LB_GETCURSEL, 0, 0);
		int count = (int)SendMessage(hwnd, LB_GETCOUNT, 0, 0);
		int scrollX = (GetWindowLong(hwnd, GWL_STYLE) & WS_HSCROLL) ? GetScrollPos(hwnd, SB_HORZ) : 0;
		bool enabled = IsWindowEnabled(hwnd) != FALSE;

		for (int i = (int)SendMessage(hwnd, LB_GETTOPINDEX, 0, 0); i >= 0 && i < count; i++)
		{
			RECT r;
			if (SendMessage(hwnd, LB_GETITEMRECT, i, (LPARAM)&r) == LB_ERR || r.top >= client.bottom)
			{
				break;
			}
			bool selected = multi ? SendMessage(hwnd, LB_GETSEL, i, 0) > 0 : i == current;
			if (selected)
			{
				RECT pill = { r.left + 2, r.top + 1, client.right - 2, r.bottom - 1 };
				FluentDialog::FillRounded(dc, pill, Scale(hwnd, 4), Mix(c.field, c.ink, 12));
				RECT bar = { pill.left, pill.top + (pill.bottom - pill.top) / 4, pill.left + Scale(hwnd, 3), pill.bottom - (pill.bottom - pill.top) / 4 };
				FluentDialog::FillRounded(dc, bar, 1, c.accent);
			}

			int length = (int)SendMessage(hwnd, LB_GETTEXTLEN, i, 0);
			if (length > 0 && length != LB_ERR)
			{
				CString text;
				SendMessage(hwnd, LB_GETTEXT, i, (LPARAM)text.GetBuffer(length + 1));
				text.ReleaseBuffer();
				RECT textRect = { r.left + Scale(hwnd, 8) - scrollX, r.top, r.right + 4096, r.bottom };
				SetTextColor(dc, enabled ? c.ink : c.dim);
				DrawText(dc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_EXPANDTABS);
			}
		}

		BitBlt(target, 0, 0, client.right, client.bottom, dc, 0, 0, SRCCOPY);
		SelectObject(dc, oldFont);
		SelectObject(dc, oldBitmap);
		DeleteObject(bitmap);
		DeleteDC(dc);
	}

	LRESULT CALLBACK ListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			PaintList(hwnd, dc);
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_PRINTCLIENT:
			PaintList(hwnd, (HDC)wParam);
			return 0;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_ENABLE:
		case WM_HSCROLL:
		case WM_VSCROLL:
		case WM_MOUSEWHEEL:
		case LB_SETCURSEL:
		case LB_SETSEL:
		case LB_SELITEMRANGE:
		case LB_SETTOPINDEX:
		{
			// the list paints selection changes straight away in system colors; draw over them at once
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			return result;
		}
		case WM_MOUSEMOVE:
		{
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			// a drop-down's list selects the row under the mouse as it moves
			TCHAR cls[16] = { 0 };
			GetClassName(hwnd, cls, 16);
			if ((wParam & MK_LBUTTON) || _tcsicmp(cls, _T("ComboLBox")) == 0)
			{
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			}
			return result;
		}
		case WM_NCDESTROY:
			RemoveWindowSubclass(hwnd, ListProc, ListSubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	// the rows of a report list (text only in every list Bulb has): the soft pill and accent bar of its other lists,
	// in place of the theme's outlined box
	bool ListViewDraw(NMLVCUSTOMDRAW* draw, LRESULT& result)
	{
		HWND list = draw->nmcd.hdr.hwndFrom;
		if (GetProp(list, RowsProp) == NULL)
		{
			return false;
		}
		if (draw->nmcd.dwDrawStage == CDDS_PREPAINT)
		{
			result = CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
			return true;
		}
		if (draw->nmcd.dwDrawStage == CDDS_POSTPAINT)
		{
			// the theme draws column lines down the empty space below the last row; Windows 11 leaves it plain
			RECT client, header;
			GetClientRect(list, &client);
			GetWindowRect(ListView_GetHeader(list), &header);
			MapWindowPoints(NULL, list, (POINT*)&header, 2);
			int top = header.bottom;
			int count = ListView_GetItemCount(list);
			RECT last;
			if (count > 0 && ListView_GetItemRect(list, count - 1, &last, LVIR_BOUNDS))
			{
				top = max(top, (int)last.bottom);
			}
			if (top < client.bottom)
			{
				RECT empty = { client.left, top, client.right, client.bottom };
				FillRect(draw->nmcd.hdc, &empty, Brush(ColorsOn(GetParent(list)).field));
			}
			result = CDRF_DODEFAULT;
			return true;
		}
		if (draw->nmcd.dwDrawStage != CDDS_ITEMPREPAINT)
		{
			return false;
		}

		Palette c = ColorsOn(GetParent(list));
		HDC dc = draw->nmcd.hdc;
		int item = (int)draw->nmcd.dwItemSpec;
		RECT client, row;
		GetClientRect(list, &client);
		ListView_GetItemRect(list, item, &row, LVIR_BOUNDS);
		row.left = client.left;
		row.right = client.right;
		FillRect(dc, &row, Brush(c.field));

		bool showSelection = GetFocus() == list || (GetWindowLong(list, GWL_STYLE) & LVS_SHOWSELALWAYS);
		if (showSelection && ListView_GetItemState(list, item, LVIS_SELECTED))
		{
			RECT pill = { row.left + 2, row.top + 1, row.right - 2, row.bottom - 1 };
			FluentDialog::FillRounded(dc, pill, Scale(list, 4), Mix(c.field, c.ink, 12));
			RECT bar = { pill.left, pill.top + (pill.bottom - pill.top) / 4, pill.left + Scale(list, 3), pill.bottom - (pill.bottom - pill.top) / 4 };
			FluentDialog::FillRounded(dc, bar, 1, c.accent);
		}

		HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(list, WM_GETFONT, 0, 0));
		SetBkMode(dc, TRANSPARENT);
		SetTextColor(dc, IsWindowEnabled(list) ? c.ink : c.dim);
		int columns = Header_GetItemCount(ListView_GetHeader(list));
		for (int column = 0; column < columns; column++)
		{
			RECT cell;
			ListView_GetSubItemRect(list, item, column, LVIR_LABEL, &cell);
			TCHAR text[512] = { 0 };
			ListView_GetItemText(list, item, column, text, 512);
			cell.left += Scale(list, 8);
			cell.right -= Scale(list, 4);
			DrawText(dc, text, -1, &cell, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
		}
		SelectObject(dc, oldFont);
		result = CDRF_SKIPDEFAULT;
		return true;
	}

	const UINT_PTR HeaderSubclassId = 0xF1F3;

	// a list's column titles: quiet text on the list's own ground, a short divider between columns
	void PaintHeader(HWND hwnd, HDC target)
	{
		Palette c = ColorsOn(GetParent(GetParent(hwnd)));
		RECT client;
		GetClientRect(hwnd, &client);
		HDC dc = CreateCompatibleDC(target);
		HBITMAP bitmap = CreateCompatibleBitmap(target, max(1, (int)client.right), max(1, (int)client.bottom));
		HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
		FillRect(dc, &client, Brush(c.field));
		RECT underline = { client.left, client.bottom - 1, client.right, client.bottom };
		FillRect(dc, &underline, Brush(c.line));

		HGDIOBJ oldFont = SelectObject(dc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
		SetBkMode(dc, TRANSPARENT);
		int count = Header_GetItemCount(hwnd);
		for (int i = 0; i < count; i++)
		{
			RECT r;
			Header_GetItemRect(hwnd, i, &r);
			TCHAR text[256] = { 0 };
			HDITEM info = { 0 };
			info.mask = HDI_TEXT | HDI_FORMAT;
			info.pszText = text;
			info.cchTextMax = 256;
			Header_GetItem(hwnd, i, &info);

			if (i > 0)
			{
				RECT divider = { r.left, r.top + (r.bottom - r.top) / 4, r.left + 1, r.bottom - (r.bottom - r.top) / 4 };
				FillRect(dc, &divider, Brush(c.line));
			}
			RECT textRect = { r.left + Scale(hwnd, 8), r.top, r.right - Scale(hwnd, 4), r.bottom - 1 };
			if (info.fmt & (HDF_SORTUP | HDF_SORTDOWN))
			{
				RECT arrow = { r.right - Scale(hwnd, 20), r.top, r.right - Scale(hwnd, 4), r.bottom - 1 };
				textRect.right = arrow.left;
				HGDIOBJ textFont = SelectObject(dc, IconFont(hwnd, 10));
				SetTextColor(dc, c.dim);
				DrawText(dc, (info.fmt & HDF_SORTUP) ? _T("\xE70E") : _T("\xE70D"), 1, &arrow, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				SelectObject(dc, textFont);
			}
			SetTextColor(dc, Mix(c.dim, c.ink, 40));
			DrawText(dc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
		}

		BitBlt(target, 0, 0, client.right, client.bottom, dc, 0, 0, SRCCOPY);
		SelectObject(dc, oldFont);
		SelectObject(dc, oldBitmap);
		DeleteObject(bitmap);
		DeleteDC(dc);
	}

	LRESULT CALLBACK HeaderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		switch (msg)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			PaintHeader(hwnd, dc);
			EndPaint(hwnd, &ps);
			return 0;
		}
		case WM_PRINTCLIENT:
			PaintHeader(hwnd, (HDC)wParam);
			return 0;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_NCDESTROY:
			RemoveWindowSubclass(hwnd, HeaderProc, HeaderSubclassId);
			break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	void ApplyControl(HWND hwnd)
	{
		Palette c = ColorsOn(GetParent(hwnd));
		if (GetProp(hwnd, ThemedProp) != NULL)
		{
			return;
		}
		SetProp(hwnd, ThemedProp, (HANDLE)1);

		TCHAR cls[64] = { 0 };
		GetClassName(hwnd, cls, 64);
		TCHAR parentCls[16] = { 0 };
		GetClassName(GetParent(hwnd), parentCls, 16);

		bool richEdit = _tcsnicmp(cls, _T("RichEdit"), 8) == 0;
		if (richEdit && _tcsicmp(parentCls, _T("#32770")) != 0)
		{
			return; // a clip being edited: its text keeps the clip's own fonts and colors on its own page
		}

		// the tab strip keeps its font: its rows were measured with it and the pages are placed below them;
		// a rich edit's font would become the font of its whole text
		HFONT font = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
		if (font != NULL && _tcsicmp(cls, WC_TABCONTROL) != 0 && richEdit == false)
		{
			SendMessage(hwnd, WM_SETFONT, (WPARAM)FontLike(font), FALSE);
		}
		LPCWSTR darkTheme = c.dark ? L"DarkMode_Explorer" : NULL;

		if (_tcsicmp(cls, _T("#32770")) == 0)
		{
			SetWindowSubclass(hwnd, DialogProc, DialogSubclassId, 0);
		}
		else if (_tcsicmp(cls, TOOLBARCLASSNAME) == 0)
		{
			SetWindowSubclass(hwnd, ToolbarProc, ToolbarSubclassId, 0);
		}
		else if (_tcsicmp(cls, _T("Button")) == 0)
		{
			LONG type = GetWindowLong(hwnd, GWL_STYLE) & BS_TYPEMASK;
			bool toggle = type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_3STATE || type == BS_AUTO3STATE ||
				type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON || type == BS_GROUPBOX;
			if (type != BS_OWNERDRAW)
			{
				SetWindowTheme(hwnd, darkTheme, NULL);
			}
			// the theme draws toggle labels black, unreadable on a dark ground; push buttons get the Windows 11 shape in both modes
			if ((toggle && (c.dark || IsSettingsPage(GetParent(hwnd)))) || IsPushButton(GetWindowLong(hwnd, GWL_STYLE)))
			{
				SetWindowSubclass(hwnd, ButtonProc, ButtonSubclassId, 0);
			}
		}
		else if (_tcsicmp(cls, HOTKEY_CLASS) == 0)
		{
			// the sunken 3D frame is replaced by the rounded one PaintHotkey draws
			SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
			SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
			SetWindowSubclass(hwnd, HotkeyProc, HotkeySubclassId, 0);
		}
		else if (_tcsicmp(cls, DATETIMEPICK_CLASS) == 0)
		{
			SetWindowTheme(hwnd, c.dark ? L"DarkMode_CFD" : NULL, NULL);
			DateTime_SetMonthCalColor(hwnd, MCSC_BACKGROUND, c.field);
			DateTime_SetMonthCalColor(hwnd, MCSC_MONTHBK, c.field);
			DateTime_SetMonthCalColor(hwnd, MCSC_TEXT, c.ink);
			DateTime_SetMonthCalColor(hwnd, MCSC_TITLEBK, c.selected);
			DateTime_SetMonthCalColor(hwnd, MCSC_TITLETEXT, c.ink);
			DateTime_SetMonthCalColor(hwnd, MCSC_TRAILINGTEXT, c.dim);
			SetWindowSubclass(hwnd, DateProc, DateSubclassId, 0);
		}
		else if (_tcsnicmp(cls, _T("RichEdit"), 8) == 0)
		{
			SetWindowTheme(hwnd, darkTheme, NULL);
			SendMessage(hwnd, EM_SETBKGNDCOLOR, 0, c.field);
			CHARFORMAT2 format = { 0 };
			format.cbSize = sizeof(format);
			format.dwMask = CFM_COLOR;
			format.crTextColor = c.ink;
			SendMessage(hwnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);
			SendMessage(hwnd, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&format);
			SetWindowSubclass(hwnd, FrameProc, FrameSubclassId, 0);
		}
		else if (_tcsicmp(cls, PROGRESS_CLASS) == 0 || _tcsicmp(cls, UPDOWN_CLASS) == 0)
		{
			SetWindowTheme(hwnd, darkTheme, NULL);
		}
		else if (CMFCLinkCtrl* link = DYNAMIC_DOWNCAST(CMFCLinkCtrl, CWnd::FromHandlePermanent(hwnd)))
		{
			// Windows 11 links: the accent color, underlined only under the mouse (MFC keeps one set of link colors)
			link->m_bAlwaysUnderlineText = FALSE;
			GetGlobalData()->clrHotLinkNormalText = c.accent;
			GetGlobalData()->clrHotLinkVisitedText = c.accent;
			GetGlobalData()->clrHotLinkHoveredText = Mix(c.accent, c.ink, 25);
			link->Invalidate();
		}
		else if (CMFCPropertyGridCtrl* grid = DYNAMIC_DOWNCAST(CMFCPropertyGridCtrl, CWnd::FromHandlePermanent(hwnd)))
		{
			grid->SetCustomColors(c.field, c.ink, c.ground, c.ink, c.ground, c.dim, c.line);
			grid->EnableHeaderCtrl(FALSE); // the MFC header only draws in light system colors; the rows say what they are
			SetWindowTheme(hwnd, darkTheme, NULL); // its scroll bar
			SetWindowSubclass(hwnd, FrameProc, FrameSubclassId, 0);
		}
		else if (_tcsicmp(cls, _T("Edit")) == 0 || _tcsicmp(cls, _T("ComboBox")) == 0)
		{
			SetWindowTheme(hwnd, c.dark ? L"DarkMode_CFD" : NULL, NULL);
			if (_tcsicmp(cls, _T("Edit")) == 0)
			{
				// DarkMode_CFD leaves a field's scroll bars light; Explorer's are dark, and FrameProc draws the border either way
				if (GetWindowLong(hwnd, GWL_STYLE) & (WS_VSCROLL | WS_HSCROLL))
				{
					SetWindowTheme(hwnd, darkTheme, NULL);
				}
				SetWindowSubclass(hwnd, FrameProc, FrameSubclassId, 0);
				if (IsRoundedField(hwnd))
				{
					SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
				}
			}
			else if ((GetWindowLong(hwnd, GWL_STYLE) & (CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) == CBS_DROPDOWNLIST)
			{
				SetWindowSubclass(hwnd, ComboProc, ComboSubclassId, 0);
				// the list it opens: the same quiet frame and soft selection as Bulb's other lists
				COMBOBOXINFO info = { sizeof(info) };
				if (GetComboBoxInfo(hwnd, &info) && info.hwndList != NULL && GetProp(info.hwndList, ThemedProp) == NULL)
				{
					SetProp(info.hwndList, ThemedProp, (HANDLE)1);
					SetWindowTheme(info.hwndList, darkTheme, NULL);
					SetWindowSubclass(info.hwndList, FrameProc, FrameSubclassId, 0);
					SetWindowSubclass(info.hwndList, ListProc, ListSubclassId, 0);
				}
			}
		}
		else if (_tcsicmp(cls, _T("ListBox")) == 0)
		{
			SetWindowTheme(hwnd, darkTheme, NULL);
			SetWindowSubclass(hwnd, FrameProc, FrameSubclassId, 0);
			if ((GetWindowLong(hwnd, GWL_STYLE) & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) == 0)
			{
				SetWindowSubclass(hwnd, ListProc, ListSubclassId, 0);
			}
		}
		else if (_tcsicmp(cls, _T("ScrollBar")) == 0)
		{
			SetWindowTheme(hwnd, darkTheme, NULL);
		}
		else if (_tcsicmp(cls, WC_LISTVIEW) == 0)
		{
			SetWindowTheme(hwnd, darkTheme, NULL);
			ListView_SetBkColor(hwnd, c.field);
			ListView_SetTextBkColor(hwnd, c.field);
			ListView_SetTextColor(hwnd, c.ink);
			// Windows 11 lists show the selected row, never a dotted box around it
			SendMessage(hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);
			ListView_SetExtendedListViewStyleEx(hwnd, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
			SetWindowSubclass(hwnd, FrameProc, FrameSubclassId, 0);
			HWND header = ListView_GetHeader(hwnd);
			if (header != NULL)
			{
				SetWindowTheme(header, c.dark ? L"DarkMode_ItemsView" : NULL, NULL);
				SetWindowSubclass(header, HeaderProc, HeaderSubclassId, 0);
			}
			// the rows are drawn in its dialog's NM_CUSTOMDRAW (ListViewDraw); only plain text reports, the only kind Bulb has
			LONG style = GetWindowLong(hwnd, GWL_STYLE);
			if ((style & LVS_TYPEMASK) == LVS_REPORT && ListView_GetImageList(hwnd, LVSIL_SMALL) == NULL &&
				(ListView_GetExtendedListViewStyle(hwnd) & LVS_EX_CHECKBOXES) == 0)
			{
				SetProp(hwnd, RowsProp, (HANDLE)1);
			}
		}
		else if (_tcsicmp(cls, WC_TREEVIEW) == 0)
		{
			SetWindowTheme(hwnd, darkTheme, NULL);
			TreeView_SetBkColor(hwnd, c.field);
			TreeView_SetTextColor(hwnd, c.ink);
			SetWindowSubclass(hwnd, FrameProc, FrameSubclassId, 0);
		}
		else if (_tcsicmp(cls, WC_TABCONTROL) == 0)
		{
			SetWindowSubclass(hwnd, TabProc, TabSubclassId, 0);
		}
		else if (GetWindow(hwnd, GW_CHILD) != NULL && _tcsicmp(cls, WC_COMBOBOX) != 0)
		{
			// any other window that holds controls (the editor's panes): Bulb's ground and colors for what it holds
			SetWindowSubclass(hwnd, DialogProc, DialogSubclassId, 0);
		}
	}

	void ApplyTree(HWND root)
	{
		ApplyControl(root);
		EnumChildWindows(root, [](HWND child, LPARAM) -> BOOL
		{
			ApplyControl(child);
			return TRUE;
		}, 0);
		RedrawWindow(root, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
	}
}

using namespace FluentInternal;

namespace
{
	HHOOK s_cbtHook = NULL;
	const UINT_PTR TooltipSubclassId = 0xF1EE;

	// a tooltip takes the dark theme once it exists; before WM_CREATE it has nothing to theme
	LRESULT CALLBACK TooltipProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
	{
		if (msg == WM_CREATE)
		{
			LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
			SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
			RemoveWindowSubclass(hwnd, TooltipProc, TooltipSubclassId);
			return result;
		}
		if (msg == WM_NCDESTROY)
		{
			RemoveWindowSubclass(hwnd, TooltipProc, TooltipSubclassId);
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}

	// every dialog Bulb opens (its own templates and message boxes) is themed as it is first activated,
	// and every tooltip follows the dark theme
	LRESULT CALLBACK CbtProc(int code, WPARAM wParam, LPARAM lParam)
	{
		if (code == HCBT_CREATEWND)
		{
			HWND hwnd = (HWND)wParam;
			TCHAR cls[32] = { 0 };
			GetClassName(hwnd, cls, 32);
			CREATESTRUCT* create = ((CBT_CREATEWND*)lParam)->lpcs;
			const RECT* page = create->hwndParent != NULL ? (const RECT*)GetProp(create->hwndParent, PageRectProp) : NULL;
			if (_tcsicmp(cls, TOOLTIPS_CLASS) == 0 && Colors().dark)
			{
				SetWindowSubclass(hwnd, TooltipProc, TooltipSubclassId, 0);
			}
			else if (page != NULL && _tcsicmp(cls, _T("#32770")) == 0 && (create->style & WS_CHILD))
			{
				// a settings page being created: born in its place on the card, and kept there from its first move
				create->x = page->left;
				create->y = page->top;
				create->cx = page->right - page->left;
				create->cy = page->bottom - page->top;
				SetWindowSubclass(hwnd, DialogProc, DialogSubclassId, 0);
			}
		}
		else if (code == HCBT_ACTIVATE)
		{
			HWND hwnd = (HWND)wParam;
			TCHAR cls[16] = { 0 };
			GetClassName(hwnd, cls, 16);
			HINSTANCE owner = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
			// the font and color pickers are classic dialogs and theme like Bulb's own;
			// file pickers host Explorer's view, belong to Windows and keep its look
			bool shellView = false;
			EnumChildWindows(hwnd, [](HWND child, LPARAM found) -> BOOL
			{
				TCHAR name[32] = { 0 };
				GetClassName(child, name, 32);
				if (_tcsicmp(name, _T("DirectUIHWND")) == 0 || _tcsicmp(name, _T("SHELLDLL_DefView")) == 0 || _tcsicmp(name, _T("DUIViewWndClassName")) == 0)
				{
					*(bool*)found = true;
					return FALSE;
				}
				return TRUE;
			}, (LPARAM)&shellView);
			bool ours = owner == AfxGetInstanceHandle() || owner == AfxGetResourceHandle() || owner == GetModuleHandle(_T("user32.dll")) ||
				(owner == GetModuleHandle(_T("comdlg32.dll")) && shellView == false);
			if (_tcsicmp(cls, _T("#32770")) == 0 && ours && GetProp(hwnd, ThemedProp) == NULL)
			{
				FluentDialog::Apply(hwnd);
			}
		}
		return CallNextHookEx(s_cbtHook, code, wParam, lParam);
	}
}

void FluentDialog::ThemeAllDialogs()
{
	if (s_cbtHook == NULL)
	{
		ApplyMenuTheme(Colors().dark); // lets DarkMode_* themes take effect on windows themed before any dialog opens
		s_cbtHook = SetWindowsHookEx(WH_CBT, CbtProc, NULL, GetCurrentThreadId());
	}
}

bool FluentDialog::IsDark()
{
	return Colors().dark;
}

COLORREF FluentDialog::Mix(COLORREF a, COLORREF b, int percentOfB)
{
	return RGB((GetRValue(a) * (100 - percentOfB) + GetRValue(b) * percentOfB) / 100,
		(GetGValue(a) * (100 - percentOfB) + GetGValue(b) * percentOfB) / 100,
		(GetBValue(a) * (100 - percentOfB) + GetBValue(b) * percentOfB) / 100);
}

int FluentDialog::Luma(COLORREF color)
{
	return (GetRValue(color) * 299 + GetGValue(color) * 587 + GetBValue(color) * 114) / 1000;
}

FluentDialog::Palette FluentDialog::ColorsFor(HWND surface)
{
	return ColorsOn(surface);
}

namespace
{
	void RoundedPath(Gdiplus::GraphicsPath& path, Gdiplus::REAL l, Gdiplus::REAL t, Gdiplus::REAL r, Gdiplus::REAL b, int radius)
	{
		Gdiplus::REAL d = (Gdiplus::REAL)radius * 2;
		path.AddArc(l, t, d, d, 180, 90);
		path.AddArc(r - d, t, d, d, 270, 90);
		path.AddArc(r - d, b - d, d, d, 0, 90);
		path.AddArc(l, b - d, d, d, 90, 90);
		path.CloseFigure();
	}
}

void FluentDialog::FillRounded(HDC dc, const RECT& rect, int radius, COLORREF color)
{
	Gdiplus::Graphics g(dc);
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	Gdiplus::GraphicsPath path;
	RoundedPath(path, (Gdiplus::REAL)rect.left, (Gdiplus::REAL)rect.top, (Gdiplus::REAL)rect.right - 1, (Gdiplus::REAL)rect.bottom - 1, radius);
	Gdiplus::SolidBrush brush(Gdi(color));
	g.FillPath(&brush, &path);
}

void FluentDialog::OutlineRounded(HDC dc, const RECT& rect, int radius, COLORREF color)
{
	Gdiplus::Graphics g(dc);
	g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	Gdiplus::GraphicsPath path;
	RoundedPath(path, rect.left + 0.5f, rect.top + 0.5f, rect.right - 1.5f, rect.bottom - 1.5f, radius);
	Gdiplus::Pen pen(Gdi(color), 1.0f);
	g.DrawPath(&pen, &path);
}

void FluentDialog::MarkLink(HWND link)
{
	SetProp(link, LinkProp, (HANDLE)1);
	InvalidateRect(link, NULL, TRUE);
}

void FluentDialog::UseContentDialog(HWND dialog, int footerTop)
{
	SetProp(dialog, CardProp, (HANDLE)(INT_PTR)(footerTop + 1));
}

void FluentDialog::UseSettingsPages(HWND sheet, const RECT* pageRect)
{
	if (pageRect == NULL)
	{
		RemoveProp(sheet, PageRectProp);
		return;
	}
	SetProp(sheet, PageRectProp, (HANDLE)pageRect);
	// pages that already exist move now; later ones are placed as they are created (CbtProc)
	EnumChildWindows(sheet, [](HWND child, LPARAM param) -> BOOL
	{
		TCHAR cls[16] = { 0 };
		GetClassName(child, cls, 16);
		if (GetParent(child) == (HWND)param && _tcsicmp(cls, _T("#32770")) == 0)
		{
			SetWindowSubclass(child, DialogProc, DialogSubclassId, 0);
			SetWindowPos(child, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE); // DialogProc supplies the place
		}
		return TRUE;
	}, (LPARAM)sheet);
}

void FluentDialog::Apply(HWND dialog)
{
	Palette c = Colors();
	ApplyMenuTheme(c.dark); // lets the DarkMode_* control themes take effect

	HWND top = GetAncestor(dialog, GA_ROOT);
	BOOL dark = c.dark ? TRUE : FALSE;
	DwmSetWindowAttribute(top, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark));
	DwmSetWindowAttribute(top, 35 /*DWMWA_CAPTION_COLOR*/, &c.ground, sizeof(c.ground));

	// windows that set no icon of their own show Windows' blank window icon in the title bar
	if (SendMessage(top, WM_GETICON, ICON_SMALL, 0) == 0 && SendMessage(top, WM_GETICON, ICON_BIG, 0) == 0 &&
		(HINSTANCE)GetWindowLongPtr(top, GWLP_HINSTANCE) != GetModuleHandle(_T("user32.dll")))
	{
		HICON icon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
		SendMessage(top, WM_SETICON, ICON_SMALL, (LPARAM)icon);
		SendMessage(top, WM_SETICON, ICON_BIG, (LPARAM)icon);
	}

	ApplyTree(dialog);
}
