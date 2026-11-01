#include "stdafx.h"
#include "CP_Main.h"
#include "GlyphButton.h"

BOOL CGlyphButton::Create(LPCTSTR accessibleName, CWnd* pParent, UINT id, CDPI* dpi)
{
	m_dpi = dpi;
	return CButton::Create(accessibleName, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CRect(0, 0, 0, 0), pParent, id);
}

void CGlyphButton::SetGlyph(WCHAR glyph)
{
	m_glyph = glyph;
	Invalidate();
}

static int Luma(COLORREF c)
{
	return (GetRValue(c) * 299 + GetGValue(c) * 587 + GetBValue(c) * 114) / 1000;
}

void CGlyphButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect = lpDrawItemStruct->rcItem;
	bool pressed = (lpDrawItemStruct->itemState & ODS_SELECTED) != 0;
	bool disabled = (lpDrawItemStruct->itemState & ODS_DISABLED) != 0;
	COLORREF ground = CGetSetOptions::m_Theme.MainWindowBG();
	COLORREF ink = CGetSetOptions::m_Theme.ListBoxEvenRowsText();

	pDC->SetBkMode(TRANSPARENT);

	if (m_glyph == 0)
	{
		pDC->FillSolidRect(rect, ground);

		COLORREF pill = m_accent ? CGetSetOptions::m_Theme.SearchTextBoxFocusBorder() : CGetSetOptions::m_Theme.SearchTextBoxFocusBG();
		if (pressed)
		{
			pill = Luma(pill) > 128 ? RGB(GetRValue(pill) * 9 / 10, GetGValue(pill) * 9 / 10, GetBValue(pill) * 9 / 10)
				: RGB(min(255, GetRValue(pill) + 18), min(255, GetGValue(pill) + 18), min(255, GetBValue(pill) + 18));
		}

		CBrush brush(pill);
		CPen pen(PS_SOLID, 1, m_accent ? pill : CGetSetOptions::m_Theme.ListBoxSelectedNoFocusBG());
		CBrush* oldBrush = pDC->SelectObject(&brush);
		CPen* oldPen = pDC->SelectObject(&pen);
		pDC->RoundRect(rect, CPoint(m_dpi->Scale(8), m_dpi->Scale(8)));
		pDC->SelectObject(oldBrush);
		pDC->SelectObject(oldPen);

		static CString textFace = FontInstalled(_T("Segoe UI Variable Text")) ? _T("Segoe UI Variable Text") : _T("Segoe UI");
		CFont font;
		font.CreateFont(-m_dpi->Scale(13), 0, 0, 0, m_accent ? FW_SEMIBOLD : FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, textFace);
		CFont* oldFont = pDC->SelectObject(&font);

		COLORREF text = m_accent ? (Luma(pill) > 140 ? RGB(0, 0, 0) : RGB(255, 255, 255)) : ink;
		pDC->SetTextColor(disabled ? CGetSetOptions::m_Theme.ListSmallQuickPasteIndexColor() : text);
		CString caption;
		GetWindowText(caption);
		pDC->DrawText(caption, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		pDC->SelectObject(oldFont);
		return;
	}

	COLORREF background = ground;
	if (pressed)
	{
		background = CGetSetOptions::m_Theme.ListBoxSelectedNoFocusBG();
	}
	pDC->FillSolidRect(rect, background);

	// Segoe Fluent Icons ships with Windows 11, Segoe MDL2 Assets with Windows 10; the glyphs used here are in both
	static CString iconFont = FontInstalled(_T("Segoe Fluent Icons")) ? _T("Segoe Fluent Icons") : _T("Segoe MDL2 Assets");

	CFont font;
	font.CreateFont(-m_dpi->Scale(14), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, iconFont);
	CFont* oldFont = pDC->SelectObject(&font);

	pDC->SetTextColor(disabled ? CGetSetOptions::m_Theme.ListSmallQuickPasteIndexColor() : ink);
	WCHAR glyph[2] = { m_glyph, 0 };
	pDC->DrawText(glyph, 1, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	pDC->SelectObject(oldFont);
}
