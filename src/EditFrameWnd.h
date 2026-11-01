#pragma once

#include "EditWnd.h"
#include "ClipIds.h"
#include "SnapWindow.h"

class CEditFrameWnd : public CFrameWnd
{
	DECLARE_DYNCREATE(CEditFrameWnd)
	CEditFrameWnd();

	bool EditIds(CClipIDs &Ids);
	void SetNotifyWnd(HWND hWnd)	{ m_hNotifyWnd = hWnd; }
	bool CloseAll();

protected:
	virtual ~CEditFrameWnd();

	CEditWnd m_EditWnd;
	HWND m_hNotifyWnd;
	SnapWindow m_snap;

protected:
	DECLARE_MESSAGE_MAP()

	void MoveControls();

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDummy();
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
public:
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnClose();
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnEnterSizeMove();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};
