#pragma once

#include <vector>

// Manage Tags window: every tag with its clip count; create, rename (merging into an existing name) and delete
class CTagManager : public CDialog
{
public:
	CTagManager(CWnd* pParent = NULL);

	enum { IDD = IDD_TAG_MANAGER };

protected:
	CListBox m_list;
	std::vector<std::pair<CString, int>> m_tags; // same order as the list box rows

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	void Fill(const CString& select);
	CString SelectedTag();
	// asks for a one-word name; empty when cancelled
	CString AskName(const CString& title, const CString& current);

	afx_msg void OnNew();
	afx_msg void OnRename();
	afx_msg void OnDelete();
	afx_msg void OnSelectionChange();
	DECLARE_MESSAGE_MAP()
};
