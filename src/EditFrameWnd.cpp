// EditFrameWnd.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "EditFrameWnd.h"
#include ".\editframewnd.h"
#include "FluentDialog.h"

// CEditFrameWnd

IMPLEMENT_DYNCREATE(CEditFrameWnd, CFrameWnd)

CEditFrameWnd::CEditFrameWnd()
{
	m_bAutoMenuEnable = FALSE;
}

CEditFrameWnd::~CEditFrameWnd()
{
}


BEGIN_MESSAGE_MAP(CEditFrameWnd, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_COMMAND(ID_BUTTON_SAVE, OnDummy)
	ON_COMMAND(ID_BUTTON_SAVE_ALL, OnDummy)
	ON_COMMAND(ID_BUTTON_CLOSE, OnDummy)
	ON_COMMAND(ID_BUTTON_NEW, OnDummy)
	ON_COMMAND(ID_BUTTON_SAVE_CLOSE_CLIPBOARD, OnDummy)
	ON_WM_SETFOCUS()
	ON_WM_CLOSE()
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_WM_MOVING()
	ON_WM_ENTERSIZEMOVE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CEditFrameWnd message handlers

int CEditFrameWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	// the standard Windows 11 title bar (rounded corners, snap layouts), colored by FluentDialog::Apply
	ModifyStyleEx(WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);

	CString csTitle = theApp.m_Language.GetString("Ditto_Edit", "Bulb Edit");
	m_EditWnd.Create(NULL, csTitle, WS_CHILD, CRect(0, 0, 0, 0), this, 100, NULL);
	m_EditWnd.ShowWindow(SW_SHOW);

	MoveControls();

	SetWindowText(_T("Bulb Editor"));
	FluentDialog::Apply(m_hWnd);

	return 0;
}

void CEditFrameWnd::OnDestroy()
{
	CFrameWnd::OnDestroy();

	CRect rect;
	GetWindowRect(&rect);
	CGetSetOptions::SetEditWndSize(rect.Size());
	CGetSetOptions::SetEditWndPoint(rect.TopLeft());

	::SendMessage(m_hNotifyWnd, WM_EDIT_WND_CLOSING, 0, 0);
}

void CEditFrameWnd::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	MoveControls();
}

void CEditFrameWnd::MoveControls()
{
	if(::IsWindow(m_EditWnd.GetSafeHwnd()))
	{
		CRect cr;
		GetClientRect(cr);
		m_EditWnd.MoveWindow(cr);
	}
}

bool CEditFrameWnd::EditIds(CClipIDs &Ids)
{
	return m_EditWnd.EditIds(Ids);
}

BOOL CEditFrameWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	if(cs.hMenu!=NULL)
	{
		::DestroyMenu(cs.hMenu);      // delete menu if loaded
		cs.hMenu = NULL;              // no menu for this window
	}

	WNDCLASS wc;
	wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = AfxWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = AfxGetInstanceHandle();
	wc.hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL; // the edit window covers the client area
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = _T("Bulb Edit Wnd");

	// Create the QPaste window class
	if (!AfxRegisterClass(&wc))
		return FALSE;

	cs.lpszClass = wc.lpszClassName;
	cs.style &= ~FWS_ADDTOTITLE;

	return CFrameWnd::PreCreateWindow(cs);
}

// the class has no brush; while the edit window is being created or resized the frame shows the theme's ground, not white
BOOL CEditFrameWnd::OnEraseBkgnd(CDC* pDC)
{
	CRect r;
	GetClientRect(r);
	pDC->FillSolidRect(r, CGetSetOptions::m_Theme.MainWindowBG());
	return TRUE;
}

void CEditFrameWnd::OnDummy()
{

}


void CEditFrameWnd::OnSetFocus(CWnd* pOldWnd)
{
	CFrameWnd::OnSetFocus(pOldWnd);

	m_EditWnd.SetFocus();
}

void CEditFrameWnd::OnClose()
{
	if(m_EditWnd.CloseEdits(true) == false)
		return;

	CFrameWnd::OnClose();
}

bool CEditFrameWnd::CloseAll()
{
	return m_EditWnd.CloseEdits(true);
}

LRESULT CEditFrameWnd::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	int dpi = HIWORD(wParam);
	m_EditWnd.OnDpiChanged(this, dpi);

	RECT* const prcNewWindow = (RECT*)lParam;
	SetWindowPos(NULL,
		prcNewWindow->left,
		prcNewWindow->top,
		prcNewWindow->right - prcNewWindow->left,
		prcNewWindow->bottom - prcNewWindow->top,
		SWP_NOZORDER | SWP_NOACTIVATE);

	log(StrF(_T("CEditFrameWnd::OnDpiChanged dpi: %d width: %d, height: %d"), dpi, (prcNewWindow->right - prcNewWindow->left), (prcNewWindow->bottom - prcNewWindow->top)));

	this->Invalidate();
	this->UpdateWindow();

	return TRUE;
}

void CEditFrameWnd::OnMoving(UINT fwSide, LPRECT pRect)
{
	CWnd::OnMoving(fwSide, pRect);
	m_snap.OnSnapMoving(m_hWnd, pRect);
}

void CEditFrameWnd::OnEnterSizeMove()
{
	m_snap.OnSnapEnterSizeMove(m_hWnd);
	CWnd::OnEnterSizeMove();
}
