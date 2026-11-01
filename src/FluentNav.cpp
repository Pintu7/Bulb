#include "stdafx.h"
#include "CP_Main.h"
#include "FluentNav.h"
#include "FluentDialog.h"

BEGIN_MESSAGE_MAP(CFluentNav, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

BOOL CFluentNav::Create(CWnd* parent, const CRect& rect, UINT id)
{
	CString cls = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(NULL, IDC_ARROW));
	return CWnd::Create(cls, _T(""), WS_CHILD | WS_VISIBLE | WS_TABSTOP, rect, parent, id);
}

void CFluentNav::SetItems(const std::vector<Item>& items)
{
	m_items = items;
	Announce();
	Invalidate();
}

void CFluentNav::SetSelected(int index)
{
	if (index >= 0 && index < (int)m_items.size() && index != m_selected)
	{
		m_selected = index;
		Announce();
		Invalidate();
	}
}

// the window's name is the open area, so screen readers say where the navigation is when it changes
void CFluentNav::Announce()
{
	if (m_selected >= 0 && m_selected < (int)m_items.size())
	{
		SetWindowText(m_items[m_selected].text);
		::NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
	}
}

int CFluentNav::Scale(int value) const
{
	UINT dpi = ::GetDpiForWindow(m_hWnd);
	return MulDiv(value, dpi ? dpi : 96, 96);
}

CRect CFluentNav::RowRect(int index) const
{
	CRect client;
	GetClientRect(client);
	int pitch = RowHeight() + Scale(4);
	return CRect(client.left, index * pitch, client.right, index * pitch + RowHeight());
}

int CFluentNav::RowAt(CPoint point) const
{
	for (int i = 0; i < (int)m_items.size(); i++)
	{
		if (RowRect(i).PtInRect(point))
		{
			return i;
		}
	}
	return -1;
}

void CFluentNav::Choose(int index)
{
	if (index < 0 || index >= (int)m_items.size())
	{
		return;
	}
	bool changed = index != m_selected;
	SetSelected(index);
	if (changed)
	{
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), NavChanged), (LPARAM)m_hWnd);
	}
}

BOOL CFluentNav::OnEraseBkgnd(CDC*)
{
	return TRUE;
}

void CFluentNav::OnPaint()
{
	CPaintDC paint(this);
	CRect client;
	GetClientRect(client);

	CDC dc;
	dc.CreateCompatibleDC(&paint);
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&paint, client.Width(), client.Height());
	CBitmap* oldBitmap = dc.SelectObject(&bitmap);

	FluentDialog::Palette c = FluentDialog::ColorsFor(GetParent()->m_hWnd);
	dc.FillSolidRect(client, c.ground);

	// the rows share the right-click menu's type and icon sizes
	static CString textFace = FontInstalled(_T("Segoe UI Variable Text")) ? _T("Segoe UI Variable Text") : _T("Segoe UI");
	static CString iconFace = FontInstalled(_T("Segoe Fluent Icons")) ? _T("Segoe Fluent Icons") : _T("Segoe MDL2 Assets");
	CFont textFont, iconFont;
	textFont.CreateFont(-Scale(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, textFace);
	iconFont.CreateFont(-Scale(16), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, iconFace);
	dc.SetBkMode(TRANSPARENT);

	// rows lift one step from the window ground, as the menu's hover lifts from the menu
	COLORREF hover = c.navHover;
	COLORREF selected = c.navSelected;

	for (int i = 0; i < (int)m_items.size(); i++)
	{
		CRect row = RowRect(i);
		bool isSelected = i == m_selected;
		if (isSelected || i == m_hot)
		{
			FluentDialog::FillRounded(dc.m_hDC, row, Scale(4), isSelected ? selected : hover);
		}
		if (isSelected)
		{
			CRect bar(row.left, row.CenterPoint().y - Scale(8), row.left + Scale(3), row.CenterPoint().y + Scale(8));
			FluentDialog::FillRounded(dc.m_hDC, bar, Scale(1), c.accent);
		}
		if (isSelected && m_keyboardCues && GetFocus() == this)
		{
			FluentDialog::OutlineRounded(dc.m_hDC, row, Scale(4), c.ink);
		}

		dc.SetTextColor(c.ink);
		CFont* oldFont = dc.SelectObject(&iconFont);
		CRect icon(row.left + Scale(14), row.top, row.left + Scale(14) + Scale(16), row.bottom);
		dc.DrawText(CString(m_items[i].glyph), icon, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		dc.SelectObject(&textFont);
		CRect text(row.left + Scale(46), row.top, row.right - Scale(8), row.bottom);
		dc.DrawText(m_items[i].text, text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
		dc.SelectObject(oldFont);
	}

	paint.BitBlt(0, 0, client.Width(), client.Height(), &dc, 0, 0, SRCCOPY);
	dc.SelectObject(oldBitmap);
}

void CFluentNav::OnMouseMove(UINT flags, CPoint point)
{
	int hot = RowAt(point);
	if (hot != m_hot)
	{
		m_hot = hot;
		TRACKMOUSEEVENT track = { sizeof(track), TME_LEAVE, m_hWnd, 0 };
		TrackMouseEvent(&track);
		Invalidate();
	}
	CWnd::OnMouseMove(flags, point);
}

LRESULT CFluentNav::OnMouseLeave(WPARAM, LPARAM)
{
	m_hot = -1;
	Invalidate();
	return 0;
}

void CFluentNav::OnLButtonDown(UINT flags, CPoint point)
{
	m_keyboardCues = false;
	SetFocus();
	Choose(RowAt(point));
	CWnd::OnLButtonDown(flags, point);
}

void CFluentNav::OnKeyDown(UINT key, UINT repeat, UINT flags)
{
	int count = (int)m_items.size();
	int next = m_selected;
	switch (key)
	{
	case VK_UP: next = max(0, m_selected - 1); break;
	case VK_DOWN: next = min(count - 1, m_selected + 1); break;
	case VK_HOME: next = 0; break;
	case VK_END: next = count - 1; break;
	default:
		CWnd::OnKeyDown(key, repeat, flags);
		return;
	}
	m_keyboardCues = true;
	Choose(next);
	Invalidate();
}

UINT CFluentNav::OnGetDlgCode()
{
	return DLGC_WANTARROWS;
}

void CFluentNav::OnSetFocus(CWnd* oldWnd)
{
	CWnd::OnSetFocus(oldWnd);
	if ((GetKeyState(VK_LBUTTON) & 0x8000) == 0)
	{
		m_keyboardCues = true; // reached with Tab
	}
	Invalidate();
}

void CFluentNav::OnKillFocus(CWnd* newWnd)
{
	CWnd::OnKillFocus(newWnd);
	Invalidate();
}
