#if !defined(AFX_GROUPNAME_H__C0A39EC3_DC62_43FF_A9BF_9AC3E356C498__INCLUDED_)
#define AFX_GROUPNAME_H__C0A39EC3_DC62_43FF_A9BF_9AC3E356C498__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupName.h : header file
//

#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CGroupName dialog

class CGroupName : public CDialog
{
// Construction
public:
	CGroupName(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGroupName)
	enum { IDD = IDD_GROUP_NAME };
	CString	m_csName;
	//}}AFX_DATA

	// optional: window title, and words completed inline as the last word is typed (used for tags)
	CString m_title;
	std::vector<CString> m_completions;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupName)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroupName)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg void OnNameChange();
	afx_msg void OnNameFocus();
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT flags, CPoint point);
	afx_msg HITTEST_RET OnNcHitTest(CPoint point);
	DECLARE_MESSAGE_MAP()

	int m_lastNameLength = 0;
	bool m_completing = false;

	// Windows 11 content dialog: title and hint inside, a large text box, the buttons in a footer band
	CString m_heading;
	CString m_hint;
	CFont m_titleFont;
	CFont m_bodyFont;
	CRect m_titleRect;
	CRect m_hintRect;
	CRect m_fieldRect;
	int m_footerTop = 0;
	int Scale(int value);
	void LayoutAsContentDialog();
	void PlaceForScale();
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPNAME_H__C0A39EC3_DC62_43FF_A9BF_9AC3E356C498__INCLUDED_)
