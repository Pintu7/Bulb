#include "stdafx.h"
#include "CP_Main.h"
#include "TagManager.h"
#include "FluentDialog.h"
#include "ClipTags.h"
#include "GroupName.h"

BEGIN_MESSAGE_MAP(CTagManager, CDialog)
	ON_BN_CLICKED(IDC_TAG_NEW, OnNew)
	ON_BN_CLICKED(IDC_TAG_RENAME, OnRename)
	ON_BN_CLICKED(IDC_TAG_DELETE, OnDelete)
	ON_LBN_DBLCLK(IDC_TAG_LIST, OnRename)
	ON_LBN_SELCHANGE(IDC_TAG_LIST, OnSelectionChange)
END_MESSAGE_MAP()

CTagManager::CTagManager(CWnd* pParent)
	: CDialog(CTagManager::IDD, pParent)
{
}

void CTagManager::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAG_LIST, m_list);
}

BOOL CTagManager::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(theApp.m_Language.GetString(_T("ManageTagsTitle"), _T("Manage Tags")));
	Fill(_T(""));
	FluentDialog::Apply(m_hWnd);

	return TRUE;
}

void CTagManager::Fill(const CString& select)
{
	try
	{
		m_tags = ClipTags::NamesWithCounts();
	}
	CATCH_SQLITE_EXCEPTION

	m_list.ResetContent();
	for (const auto& tag : m_tags)
	{
		m_list.AddString(StrF(_T("#%s  (%d)"), (LPCTSTR)tag.first, tag.second));
	}

	for (int i = 0; i < (int)m_tags.size(); i++)
	{
		if (m_tags[i].first == select)
		{
			m_list.SetCurSel(i);
		}
	}

	OnSelectionChange();
}

CString CTagManager::SelectedTag()
{
	int row = m_list.GetCurSel();
	return (row >= 0 && row < (int)m_tags.size()) ? m_tags[row].first : _T("");
}

void CTagManager::OnSelectionChange()
{
	BOOL selected = SelectedTag().IsEmpty() == FALSE;
	GetDlgItem(IDC_TAG_RENAME)->EnableWindow(selected);
	GetDlgItem(IDC_TAG_DELETE)->EnableWindow(selected);
}

CString CTagManager::AskName(const CString& title, const CString& current)
{
	CGroupName dlg(this);
	dlg.m_title = title;
	dlg.m_csName = current;

	while (dlg.DoModal() == IDOK)
	{
		std::vector<CString> names = ClipTags::Parse(dlg.m_csName);
		if (names.size() == 1)
		{
			return names[0];
		}

		MessageBox(theApp.m_Language.GetString(_T("TagNameOneWord"), _T("A tag name is a single word.")), _T("Bulb"), MB_OK);
	}

	return _T("");
}

void CTagManager::OnNew()
{
	CString name = AskName(theApp.m_Language.GetString(_T("NewTag"), _T("New tag")), _T(""));
	if (name.IsEmpty())
	{
		return;
	}

	try
	{
		ClipTags::CreateTag(name);
	}
	CATCH_SQLITE_EXCEPTION

	Fill(name);
}

void CTagManager::OnRename()
{
	CString tag = SelectedTag();
	if (tag.IsEmpty())
	{
		return;
	}

	CString name = AskName(StrF(_T("%s #%s"), theApp.m_Language.GetString(_T("RenameTagTitle"), _T("Rename")), (LPCTSTR)tag), tag);
	if (name.IsEmpty())
	{
		return;
	}

	try
	{
		ClipTags::RenameTag(tag, name);
	}
	CATCH_SQLITE_EXCEPTION

	Fill(name);
}

void CTagManager::OnDelete()
{
	int row = m_list.GetCurSel();
	CString tag = SelectedTag();
	if (tag.IsEmpty())
	{
		return;
	}

	CString question = StrF(theApp.m_Language.GetString(_T("DeleteTagConfirmCount"), _T("Delete the tag #%s?\r\n\r\nIt is removed from %d clip(s); the clips are kept.")), (LPCTSTR)tag, m_tags[row].second);
	if (MessageBox(question, _T("Bulb"), MB_OKCANCEL | MB_ICONWARNING) != IDOK)
	{
		return;
	}

	try
	{
		ClipTags::DeleteTag(tag);
	}
	CATCH_SQLITE_EXCEPTION

	// keep the selection on the row that took its place
	Fill(row < (int)m_tags.size() - 1 ? m_tags[row + 1].first : (row > 0 ? m_tags[row - 1].first : _T("")));
}
