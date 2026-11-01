#pragma once

#include "DPI.h"

// Flat button showing one Segoe Fluent Icons / MDL2 glyph in the theme's colors.
// With no glyph it is a text button: a rounded pill, filled with the accent color when it is the primary action.
class CGlyphButton : public CButton
{
public:
	BOOL Create(LPCTSTR accessibleName, CWnd* pParent, UINT id, CDPI* dpi);
	void SetGlyph(WCHAR glyph);
	void SetAccent(bool accent) { m_accent = accent; Invalidate(); }

	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	WCHAR m_glyph = 0;
	bool m_accent = false;
	CDPI* m_dpi = nullptr;
};
