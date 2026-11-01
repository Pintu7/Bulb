#pragma once

#include <vector>

// The left navigation of a Windows 11 settings window: one row per area, an icon and its name, drawn like
// Bulb's right-click menu (same font, icon size and rounded hover), the open area marked by a soft pill and
// an accent bar. Clicking a row or moving with the arrow keys sends the parent WM_COMMAND(id, NavChanged).
class CFluentNav : public CWnd
{
public:
	enum { NavChanged = 1 };

	struct Item
	{
		CString text;
		WCHAR glyph;
	};

	BOOL Create(CWnd* parent, const CRect& rect, UINT id);
	void SetItems(const std::vector<Item>& items);
	void SetSelected(int index);
	int GetSelected() const { return m_selected; }

protected:
	std::vector<Item> m_items;
	int m_selected = 0;
	int m_hot = -1;
	bool m_keyboardCues = false;

	int Scale(int value) const;
	int RowHeight() const { return Scale(36); }
	int RowAt(CPoint point) const;
	CRect RowRect(int index) const;
	void Choose(int index);
	void Announce();

	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT flags, CPoint point);
	afx_msg LRESULT OnMouseLeave(WPARAM, LPARAM);
	afx_msg void OnLButtonDown(UINT flags, CPoint point);
	afx_msg void OnKeyDown(UINT key, UINT repeat, UINT flags);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnSetFocus(CWnd* oldWnd);
	afx_msg void OnKillFocus(CWnd* newWnd);
	DECLARE_MESSAGE_MAP()
};
