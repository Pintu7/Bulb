#include "stdafx.h"
#include "CP_Main.h"
#include "TagBar.h"
#include "FluentDialog.h"
#include <algorithm>

BEGIN_MESSAGE_MAP(CTagBar, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

BOOL CTagBar::Create(CWnd* pParent, UINT id, CDPI* dpi)
{
	m_dpi = dpi;
	return CWnd::Create(AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(NULL, IDC_ARROW)), _T(""), WS_CHILD, CRect(0, 0, 0, 0), pParent, id);
}

void CTagBar::SetTags(const std::vector<CString>& tags)
{
	m_tags = tags;
	Invalidate();
}

void CTagBar::SetActiveTags(const std::vector<CString>& tags)
{
	m_activeTags = tags;
	Invalidate();
}

void CTagBar::OnPaint()
{
	CPaintDC paintDC(this);
	CRect client;
	GetClientRect(client);

	// drawn off-screen so the chips don't flicker
	CDC dc;
	dc.CreateCompatibleDC(&paintDC);
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&paintDC, client.Width(), client.Height());
	CBitmap* oldBitmap = dc.SelectObject(&bitmap);

	COLORREF background = CGetSetOptions::m_Theme.MainWindowBG();
	COLORREF text = CGetSetOptions::m_Theme.ListBoxEvenRowsText();
	dc.FillSolidRect(client, background);

	CFont font;
	font.CreateFont(-m_dpi->Scale(12), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, ModernUIFontName());
	CFont* oldFont = dc.SelectObject(&font);
	dc.SetBkMode(TRANSPARENT);

	m_chipRects.clear();
	int x = 0;
	int padding = m_dpi->Scale(9);
	int gap = m_dpi->Scale(6);

	for (const CString& tag : m_tags)
	{
		CString label = _T("#") + tag;
		int width = dc.GetTextExtent(label).cx + padding * 2;
		if (x + width > client.Width())
		{
			break;
		}

		CRect chip(x, m_dpi->Scale(2), x + width, client.Height() - m_dpi->Scale(2));
		bool active = std::find(m_activeTags.begin(), m_activeTags.end(), tag) != m_activeTags.end();
		COLORREF fill = active ? CGetSetOptions::m_Theme.ListBoxSelectedBG() : FluentDialog::Mix(background, text, 12);

		CBrush brush(fill);
		CPen pen(PS_SOLID, 1, fill);
		CBrush* oldBrush = dc.SelectObject(&brush);
		CPen* oldPen = dc.SelectObject(&pen);
		dc.RoundRect(chip, CPoint(chip.Height(), chip.Height()));
		dc.SelectObject(oldPen);
		dc.SelectObject(oldBrush);

		dc.SetTextColor(active ? CGetSetOptions::m_Theme.ListBoxSelectedText() : text);
		dc.DrawText(label, chip, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		m_chipRects.push_back(chip);
		x += width + gap;
	}

	dc.SelectObject(oldFont);
	paintDC.BitBlt(0, 0, client.Width(), client.Height(), &dc, 0, 0, SRCCOPY);
	dc.SelectObject(oldBitmap);
}

BOOL CTagBar::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

int CTagBar::ChipAt(CPoint point)
{
	for (int i = 0; i < (int)m_chipRects.size(); i++)
	{
		if (m_chipRects[i].PtInRect(point))
		{
			return i;
		}
	}

	return -1;
}

void CTagBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	int chip = ChipAt(point);
	if (chip >= 0)
	{
		m_clickedTag = m_tags[chip];
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
	}

	CWnd::OnLButtonUp(nFlags, point);
}

void CTagBar::OnRButtonUp(UINT nFlags, CPoint point)
{
	int chip = ChipAt(point);
	if (chip >= 0)
	{
		m_clickedTag = m_tags[chip];
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), TAGBAR_RIGHT_CLICK), (LPARAM)m_hWnd);
	}

	CWnd::OnRButtonUp(nFlags, point);
}

BOOL CTagBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint point;
	GetCursorPos(&point);
	ScreenToClient(&point);

	if (ChipAt(point) >= 0)
	{
		::SetCursor(::LoadCursor(NULL, IDC_HAND));
		return TRUE;
	}

	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}
