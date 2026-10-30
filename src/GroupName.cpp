// GroupName.cpp : implementation file
//

#include "stdafx.h"
#include "cp_main.h"
#include "GroupName.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupName dialog


CGroupName::CGroupName(CWnd* pParent /*=NULL*/)
	: CDialog(CGroupName::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGroupName)
	m_csName = _T("");
	//}}AFX_DATA_INIT
}


void CGroupName::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupName)
	DDX_Text(pDX, IDC_NAME, m_csName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupName, CDialog)
	//{{AFX_MSG_MAP(CGroupName)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_NAME, OnNameChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupName message handlers

void CGroupName::OnOK() 
{
	UpdateData(TRUE);
	
	CDialog::OnOK();
}

BOOL CGroupName::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (m_title.IsEmpty() == FALSE)
		SetWindowText(m_title);

	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_NAME);
	if(pEdit)
	{
		pEdit->SetFocus();
		pEdit->SetSel(-1, -1);
		m_lastNameLength = pEdit->GetWindowTextLength();
	}

	return FALSE;
}

void CGroupName::OnNameChange()
{
	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_NAME);
	if (pEdit == NULL || m_completions.empty() || m_completing)
		return;

	CString text;
	pEdit->GetWindowText(text);

	// only complete while typing forward, so backspace can remove a suggestion
	bool typedForward = text.GetLength() > m_lastNameLength;
	m_lastNameLength = text.GetLength();
	if (typedForward == false)
		return;

	int wordStart = max(text.ReverseFind(' '), text.ReverseFind(',')) + 1;
	CString word = text.Mid(wordStart);
	word.TrimLeft('#');
	word.MakeLower();
	if (word.IsEmpty())
		return;

	for (const CString& completion : m_completions)
	{
		if (completion.GetLength() > word.GetLength() &&
			completion.Left(word.GetLength()) == word)
		{
			// the suggested part stays selected, so typing on replaces it
			int typedLength = text.GetLength();
			text += completion.Mid(word.GetLength());

			m_completing = true;
			pEdit->SetWindowText(text);
			pEdit->SetSel(typedLength, text.GetLength());
			m_completing = false;
			break;
		}
	}
}
