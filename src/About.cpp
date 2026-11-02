// About.cpp : implementation file
// test test test

//Main branch test

#include "stdafx.h"
#include "cp_main.h"
#include "About.h"
#include "misc.h"
#include "FluentDialog.h"
#include "UpdateCheck.h"
#include <memory>

static const UINT WM_UPDATE_CHECKED = WM_APP + 0x42;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAbout property page

IMPLEMENT_DYNCREATE(CAbout, CPropertyPage)

CAbout::CAbout() : CPropertyPage(CAbout::IDD)
{
	m_csTitle = theApp.m_Language.GetString("AboutTitle", "About");
	m_psp.pszTitle = m_csTitle;
	m_psp.dwFlags |= PSP_USETITLE;

	//{{AFX_DATA_INIT(CAbout)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CAbout::~CAbout()
{
}

void CAbout::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAbout)
	DDX_Control(pDX, IDC_STATIC_LINK, m_Link);
	DDX_Control(pDX, IDC_HYPER_LINK, m_HyperLink);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAbout, CPropertyPage)
	//{{AFX_MSG_MAP(CAbout)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CHECK_UPDATES, OnCheckUpdates)
	ON_MESSAGE(WM_UPDATE_CHECKED, OnUpdateChecked)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAbout message handlers

BOOL CAbout::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	auto Text = [](LPCTSTR key, LPCTSTR english) { return theApp.m_Language.GetString(key, english); };

	CString version = _T("Version ") + GetVersionString(GetRunningVersion());
#if _WIN64
	version += _T(" (64-bit)");
#else
	version += _T(" (32-bit)");
#endif

	m_HyperLink.SetWindowText(Text(_T("AboutIssuesLink"), _T("Open GitHub issues")));
	m_HyperLink.SetURL(_T("https://github.com/Pintu7/Bulb/issues"));
	m_Link.SetWindowText(_T("github.com/sabrogden/Ditto"));
	m_Link.SetURL(_T("https://github.com/sabrogden/Ditto")); // credit and source of the original program (GPL-3.0)
	m_HyperLink.SetUnderline(FALSE);
	m_Link.SetUnderline(FALSE);
	FluentDialog::MarkLink(m_HyperLink);
	FluentDialog::MarkLink(m_Link);

	// Windows 11 Settings rows, in the same type as every other page
	using L = FluentDialog::SettingsLayout;
	const int link = 200;
	L layout;
	layout
		.SectionText(_T("Bulb"))
		.RowText(_T("Bulb"), {}, version + _T(" \x00B7 by Andrea Rinaldi"))
		.RowText(Text(_T("AboutUpdates"), _T("Updates")), { { IDC_CHECK_UPDATES, 160 } },
			Text(_T("AboutUpdatesDesc"), _T("Looks for a newer release on GitHub")))
		.RowText(Text(_T("AboutProgram"), _T("Program")), {}, CGetSetOptions::GetExeFileName())
		.RowText(Text(_T("AboutDatabase"), _T("Database")), {}, CGetSetOptions::GetDBPath())
		.RowText(_T("SQLite"), {}, CString(sqlite3_libversion()))
		.SectionText(Text(_T("AboutCredits"), _T("Credits")))
		.RowText(_T("Ditto"), { { IDC_STATIC_LINK, link } },
			Text(_T("AboutDittoCredit"), _T("Bulb is a fork of Ditto, the clipboard manager by Scott Brogden and Kevin Edwards, and of the work of all its contributors. Both are free software under the GPL-3.0.")))
		.SectionText(Text(_T("AboutFeedback"), _T("Feedback")))
		.RowText(Text(_T("AboutIssues"), _T("Report a bug or request a feature")), { { IDC_HYPER_LINK, link } },
			Text(_T("AboutIssuesDesc"), _T("On Bulb's page on GitHub")));

	// add-ins: "Name Ver: 1, Ver2: 1", then "    Function (what it does)" for each of its functions
	CStringArray lines;
	theApp.m_Addins.AboutScreenText(lines);
	if (lines.GetCount() > 0)
	{
		layout.SectionText(Text(_T("AboutAddins"), _T("Add-ins")));
	}
	for (INT_PTR i = 0; i < lines.GetCount(); i++)
	{
		CString line = lines[i];
		bool function = line.Left(1) == _T(" ");
		line.Trim();
		if (line.IsEmpty())
		{
			continue;
		}
		int open = line.Find(_T(" ("));
		if (function && open > 0 && line.Right(1) == _T(")"))
		{
			layout.RowText(line.Left(open), {}, line.Mid(open + 2, line.GetLength() - open - 3));
		}
		else
		{
			int ver = line.Find(_T(" Ver: "));
			CString number = ver > 0 ? line.Mid(ver + 6).SpanExcluding(_T(",")) : CString();
			layout.RowText(ver > 0 ? line.Left(ver) : line, {}, number.IsEmpty() ? CString() : _T("Version ") + number);
		}
	}
	layout.AttachTo(m_hWnd);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAbout::OnCheckUpdates()
{
	GetDlgItem(IDC_CHECK_UPDATES)->EnableWindow(FALSE);
	SetDlgItemText(IDC_CHECK_UPDATES, theApp.m_Language.GetString(_T("AboutUpdatesChecking"), _T("Checking...")));
	UpdateCheck::Start(m_hWnd, WM_UPDATE_CHECKED);
}

LRESULT CAbout::OnUpdateChecked(WPARAM, LPARAM lParam)
{
	std::unique_ptr<std::string> tag((std::string*)lParam);
	auto Text = [](LPCTSTR key, LPCTSTR english) { return theApp.m_Language.GetString(key, english); };
	SetDlgItemText(IDC_CHECK_UPDATES, Text(_T("AboutUpdatesButton"), _T("Check for updates")));
	GetDlgItem(IDC_CHECK_UPDATES)->EnableWindow(TRUE);

	VersionInfo running = GetRunningVersion();
	CString current = GetVersionString(running);
	CString message;
	if (tag == nullptr)
	{
		MessageBox(Text(_T("AboutUpdatesFailed"), _T("Could not check for updates. Check the internet connection and try again.")), _T("Bulb"), MB_OK | MB_ICONWARNING);
	}
	else if (UpdateCheck::IsNewer(*tag, running.Major, running.Minor, running.Revision))
	{
		CString newest(tag->c_str());
		newest.TrimLeft(_T("vV"));
		message.Format(Text(_T("AboutUpdatesAvailable"), _T("Bulb %s is available. You have %s.\n\nOpen the download page?")), newest.GetString(), current.GetString());
		if (MessageBox(message, _T("Bulb"), MB_YESNO | MB_ICONINFORMATION) == IDYES)
		{
			// the tag passed UpdateCheck::Parse, so it is only digits, dots and a leading v
			CHyperLink::GotoURL(_T("https://github.com/Pintu7/Bulb/releases/tag/") + CString(tag->c_str()), SW_SHOW);
		}
	}
	else
	{
		message.Format(Text(_T("AboutUpdatesLatest"), _T("You have the latest version of Bulb (%s).")), current.GetString());
		MessageBox(message, _T("Bulb"), MB_OK | MB_ICONINFORMATION);
	}
	return 0;
}
