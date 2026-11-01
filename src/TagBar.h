#pragma once

#include <vector>
#include "DPI.h"

// Row of clickable #tag chips under the clip list. Chips that don't fit are not shown; typing #tag reaches every tag.
// A click sends BN_CLICKED to the parent, which reads ClickedTag().
// notification code sent with WM_COMMAND when a chip is right-clicked (ClickedTag() says which)
#define TAGBAR_RIGHT_CLICK 100

class CTagBar : public CWnd
{
public:
	BOOL Create(CWnd* pParent, UINT id, CDPI* dpi);

	void SetTags(const std::vector<CString>& tags);
	void SetActiveTags(const std::vector<CString>& tags);
	bool HasTags() const { return m_tags.empty() == false; }
	CString ClickedTag() const { return m_clickedTag; }

protected:
	CString m_clickedTag;
	std::vector<CString> m_tags;
	std::vector<CString> m_activeTags;
	std::vector<CRect> m_chipRects; // one per drawn chip, same order as m_tags
	CDPI* m_dpi = nullptr;

	int ChipAt(CPoint point);

	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	DECLARE_MESSAGE_MAP()
};
