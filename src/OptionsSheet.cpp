// OptionsSheet.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "OptionsSheet.h"
#include "OptionsKeyBoard.h"
#include "OptionsGeneral.h"
#include "OptionsQuickPaste.h"
#include "OptionsStats.h"
#include "OptionsTypes.h"
#include "About.h"
#include "OptionFriends.h"
#include "OptionsCopyBuffers.h"
#include "Misc.h"
#include "QuickPasteKeyboard.h"
#include "FluentDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif 


/////////////////////////////////////////////////////////////////////////////
// COptionsSheet

IMPLEMENT_DYNAMIC(COptionsSheet, CPropertySheet)

COptionsSheet::COptionsSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	m_themeChanged = FALSE;
	m_pKeyBoardOptions = NULL;
	m_pGeneralOptions = NULL;
	//m_pQuickPasteOptions = NULL;
	m_pCopyBuffers = NULL;
	m_pStats = NULL;
	m_pTypes = NULL;
	m_pAbout = NULL;
	m_pFriends = NULL;
	m_pCopyBuffers = NULL;
	m_pQuickPasteShortCuts = NULL;
	
	m_pUtilites = NULL;
	m_hWndParent = NULL;

	EnableStackedTabs(TRUE);

	m_pGeneralOptions = new COptionsGeneral;
	m_pKeyBoardOptions = new COptionsKeyBoard;
	//m_pQuickPasteOptions = new COptionsQuickPaste;
	m_pQuickPasteShortCuts = new CQuickPasteKeyboard;

	m_pCopyBuffers = new COptionsCopyBuffers;
	m_pStats = new COptionsStats;
	m_pTypes = new COptionsTypes;
	m_pAbout = new CAbout;

	AddPage(m_pGeneralOptions);
	AddPage(m_pTypes);
	AddPage(m_pKeyBoardOptions);
	AddPage(m_pCopyBuffers);
	//AddPage(m_pQuickPasteOptions);
	AddPage(m_pQuickPasteShortCuts);
	if(CGetSetOptions::GetAllowFriends())
	{
		m_pFriends = new COptionFriends;
		AddPage(m_pFriends);
	}
	AddPage(m_pStats);
	AddPage(m_pAbout);

	
}

COptionsSheet::~COptionsSheet()
{
	delete m_pGeneralOptions;
	delete m_pKeyBoardOptions;
	//delete m_pQuickPasteOptions;
	delete m_pCopyBuffers;
	delete m_pStats;
	delete m_pTypes;
	delete m_pAbout;	
	delete m_pFriends;
	delete m_pUtilites;
	delete m_pQuickPasteShortCuts;	
}

BEGIN_MESSAGE_MAP(COptionsSheet, CPropertySheet)
	//{{AFX_MSG_MAP(COptionsSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_WM_DESTROY()
	ON_WM_NCDESTROY()
	ON_WM_PAINT()
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	//ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsSheet message handlers

void COptionsSheet::OnDestroy()
{
	FluentDialog::UseSettingsPages(m_hWnd, NULL);
	CPropertySheet::OnDestroy();
}

int COptionsSheet::Scale(int value)
{
	UINT dpi = ::GetDpiForWindow(m_hWnd);
	return MulDiv(value, dpi ? dpi : 96, 96);
}

// Windows 11 Settings: the tabs become a navigation column with icons, the open area's name is the title,
// and the page sits on a card one step lighter than the window
void COptionsSheet::LayoutAsSettings()
{
	CTabCtrl* tab = GetTabControl();
	CPropertyPage* active = GetActivePage();
	if (tab == NULL || active == NULL)
	{
		return;
	}

	CRect page;
	active->GetWindowRect(page);
	ScreenToClient(page);

	int edge = Scale(16);
	int navWidth = Scale(220);
	int titleHeight = Scale(56);
	int pad = Scale(12); // the pages carry their own inner margin

	int contentLeft = edge + navWidth + Scale(12);
	m_titleRect.SetRect(contentLeft, edge, contentLeft + page.Width() + pad * 2, edge + titleHeight);
	m_cardRect.SetRect(contentLeft, m_titleRect.bottom, contentLeft + page.Width() + pad * 2, m_titleRect.bottom + page.Height() + pad * 2);
	m_pageRect = m_cardRect;
	m_pageRect.DeflateRect(pad, pad);

	// the buttons sit under the card, right aligned, at the Windows 11 size
	int buttonHeight = Scale(32);
	int buttonTop = m_cardRect.bottom + edge;
	int right = m_cardRect.right;
	for (int id : { IDCANCEL, IDOK })
	{
		CWnd* button = GetDlgItem(id);
		if (button == NULL || (button->GetStyle() & WS_VISIBLE) == 0) // the sheet itself is not shown yet
		{
			continue;
		}
		CRect r;
		button->GetWindowRect(r);
		int width = max(r.Width(), Scale(120));
		button->SetWindowPos(NULL, right - width, buttonTop, width, buttonHeight, SWP_NOZORDER | SWP_NOACTIVATE);
		right -= width + Scale(8);
	}

	tab->ShowWindow(SW_HIDE);

	std::vector<CFluentNav::Item> items;
	for (int i = 0; i < GetPageCount(); i++)
	{
		TCHAR text[128] = { 0 };
		TCITEM info = { 0 };
		info.mask = TCIF_TEXT;
		info.pszText = text;
		info.cchTextMax = 128;
		tab->GetItem(i, &info);

		CPropertyPage* p = GetPage(i);
		WCHAR glyph = 0xE713; // Settings
		if (p == m_pTypes) glyph = 0xE8A5;                    // Document
		else if (p == m_pKeyBoardOptions) glyph = 0xE765;     // Keyboard
		else if (p == m_pCopyBuffers) glyph = 0xE8C8;         // Copy
		else if (p == m_pQuickPasteShortCuts) glyph = 0xE92E; // Keyboard shortcuts
		else if (p == m_pFriends) glyph = 0xE716;             // People
		else if (p == m_pStats) glyph = 0xE9D2;               // Chart
		else if (p == m_pAbout) glyph = 0xE946;               // Info
		items.push_back({ text, glyph });
	}

	CRect navRect(edge, edge, edge + navWidth, m_cardRect.bottom);
	m_nav.Create(this, navRect, 0xF1F0);
	m_nav.SetItems(items);
	m_nav.SetSelected(GetActiveIndex());
	m_nav.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); // first in the Tab order

	CRect windowRect, clientRect;
	GetWindowRect(windowRect);
	GetClientRect(clientRect);
	int clientWidth = m_cardRect.right + edge;
	int clientHeight = buttonTop + buttonHeight + edge;
	SetWindowPos(NULL, 0, 0, windowRect.Width() + clientWidth - clientRect.Width(), windowRect.Height() + clientHeight - clientRect.Height(),
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	CenterWindow();

	m_layoutDpi = ::GetDpiForWindow(m_hWnd);
	FluentDialog::UseSettingsPages(m_hWnd, &m_pageRect);
}

// moved to a monitor with another scale: Windows rescales the sheet and its controls; the places this layout keeps
// (the title, the card and where pages go) are rescaled first, so the pages are put where the new scale wants them
LRESULT COptionsSheet::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	UINT dpi = HIWORD(wParam);
	auto scale = [&](CRect& r)
	{
		r.SetRect(MulDiv(r.left, dpi, m_layoutDpi), MulDiv(r.top, dpi, m_layoutDpi), MulDiv(r.right, dpi, m_layoutDpi), MulDiv(r.bottom, dpi, m_layoutDpi));
	};
	scale(m_titleRect);
	scale(m_cardRect);
	scale(m_pageRect);
	m_layoutDpi = dpi;

	LRESULT result = Default();

	if (CPropertyPage* active = GetActivePage())
	{
		active->SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE); // the page's subclass supplies the place
	}
	Invalidate();
	return result;
}

void COptionsSheet::ShowPage(int index)
{
	if (index < 0 || index >= GetPageCount())
	{
		return;
	}
	SetActivePage(index);
	m_nav.SetSelected(index);
	InvalidateRect(m_titleRect);
	InvalidateRect(m_cardRect); // the card shows only behind pages without settings rows
}

BOOL COptionsSheet::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (::IsWindow(m_nav.m_hWnd) && (HWND)lParam == m_nav.m_hWnd && HIWORD(wParam) == CFluentNav::NavChanged)
	{
		ShowPage(m_nav.GetSelected());
		return TRUE;
	}
	return CPropertySheet::OnCommand(wParam, lParam);
}

BOOL COptionsSheet::PreTranslateMessage(MSG* pMsg)
{
	// Ctrl+Tab and Ctrl+PgUp/PgDn walk the areas, as they walked the tabs
	if (pMsg->message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000) && ::IsWindow(m_nav.m_hWnd))
	{
		bool back = (pMsg->wParam == VK_TAB && (GetKeyState(VK_SHIFT) & 0x8000)) || pMsg->wParam == VK_PRIOR;
		bool forward = (pMsg->wParam == VK_TAB && (GetKeyState(VK_SHIFT) & 0x8000) == 0) || pMsg->wParam == VK_NEXT;
		if (back || forward)
		{
			int count = GetPageCount();
			ShowPage((GetActiveIndex() + (forward ? 1 : count - 1)) % count);
			return TRUE;
		}
	}
	return CPropertySheet::PreTranslateMessage(pMsg);
}

void COptionsSheet::OnPaint()
{
	CPaintDC dc(this);
	if (m_cardRect.IsRectEmpty())
	{
		return;
	}

	// the card behind the page
	CPropertyPage* active = GetActivePage();
	// a page laid out as settings rows draws its own cards; the others sit on one card
	if (active == NULL || FluentDialog::HasSettingsLayout(active->m_hWnd) == false)
	{
		COLORREF card = FluentDialog::ColorsFor(active != NULL ? active->m_hWnd : NULL).ground;
		FluentDialog::FillRounded(dc.m_hDC, m_cardRect, Scale(8), card);
	}

	// the open area's name, as a Windows 11 Settings page title
	TCHAR text[128] = { 0 };
	TCITEM info = { 0 };
	info.mask = TCIF_TEXT;
	info.pszText = text;
	info.cchTextMax = 128;
	GetTabControl()->GetItem(GetActiveIndex(), &info);

	CFont font;
	font.CreateFont(-Scale(28), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, ModernDisplayFontName());
	CFont* oldFont = dc.SelectObject(&font);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(FluentDialog::ColorsFor(m_hWnd).ink);
	CRect title(m_titleRect);
	title.left += Scale(4);
	title.bottom -= Scale(12);
	dc.DrawText(text, -1, title, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
	dc.SelectObject(oldFont);
}

void COptionsSheet::SetNotifyWnd(HWND hWnd)
{
	m_hWndParent = hWnd;
}

BOOL COptionsSheet::OnInitDialog() 
{
	m_bModeless = FALSE;   
	m_nFlags |= WF_CONTINUEMODAL;

	HICON b = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 64, 64, LR_SHARED);
	SetIcon(b, TRUE);

	BOOL bResult = CPropertySheet::OnInitDialog();

	SetWindowText(_T("Options"));

	theApp.m_Language.UpdateOptionsSheet(this);

	::ShowWindow(::GetDlgItem(m_hWnd, ID_APPLY_NOW), SW_HIDE);

	LayoutAsSettings();
	FluentDialog::Apply(m_hWnd);

	m_bModeless = TRUE;
	m_nFlags &= ~WF_CONTINUEMODAL;

	return bResult;
}

void COptionsSheet::OnNcDestroy()
{
	CPropertySheet::OnNcDestroy();
	::PostMessage(m_hWndParent, WM_OPTIONS_CLOSED, m_themeChanged, 0);
}
