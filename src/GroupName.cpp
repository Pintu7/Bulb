// GroupName.cpp : implementation file
//

#include "stdafx.h"
#include "cp_main.h"
#include "GroupName.h"
#include "FluentDialog.h"
#include <dwmapi.h>

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
	ON_EN_SETFOCUS(IDC_NAME, OnNameFocus)
	ON_EN_KILLFOCUS(IDC_NAME, OnNameFocus)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_NCHITTEST()
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupName message handlers

void CGroupName::OnOK()
{
	UpdateData(TRUE);

	CDialog::OnOK();
}

int CGroupName::Scale(int value)
{
	UINT dpi = ::GetDpiForWindow(m_hWnd);
	return MulDiv(value, dpi ? dpi : 96, 96);
}

BOOL CGroupName::OnInitDialog()
{
	CDialog::OnInitDialog();

	LayoutAsContentDialog();

	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_NAME);
	if(pEdit)
	{
		pEdit->SetFocus();
		pEdit->SetSel(-1, -1);
		m_lastNameLength = pEdit->GetWindowTextLength();
	}

	return FALSE;
}

// Windows 11 content dialog: no title bar; the title and a hint inside; a text box at the Windows 11 size;
// the two buttons side by side, equal width, in a footer band of the window ground
void CGroupName::LayoutAsContentDialog()
{
	// "Tags (separated by spaces)" reads as a title and its hint
	CString title = m_title;
	if (title.IsEmpty())
	{
		GetDlgItemText(IDC_STATIC, title); // the template's "Name" label
	}
	m_heading = title;
	int open = title.ReverseFind('(');
	if (open > 0 && title.Right(1) == _T(")"))
	{
		CString inside = title.Mid(open + 1, title.GetLength() - open - 2);
		if (inside.SpanIncluding(_T("0123456789")) != inside) // "(3)" is a count, it stays in the title
		{
			m_heading = title.Left(open);
			m_heading.TrimRight();
			m_hint = inside.Left(1).MakeUpper() + inside.Mid(1);
		}
	}
	SetWindowText(title); // still names the window in Alt+Tab
	if (CWnd* label = GetDlgItem(IDC_STATIC))
	{
		label->ShowWindow(SW_HIDE);
	}

	ModifyStyle(WS_CAPTION | WS_SYSMENU | DS_MODALFRAME, 0);
	ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, 0);

	// the edit has no frame of its own: it sits inside the rounded box OnPaint draws
	if (CWnd* edit = GetDlgItem(IDC_NAME))
	{
		edit->ModifyStyle(WS_BORDER, 0);
		edit->ModifyStyleEx(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, 0, SWP_FRAMECHANGED);
	}

	PlaceForScale();
	CenterWindow();

	DWORD round = 2; // DWMWCP_ROUND, like every Windows 11 dialog
	DwmSetWindowAttribute(m_hWnd, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &round, sizeof(round));
	COLORREF border = FluentDialog::ColorsFor(NULL).line;
	DwmSetWindowAttribute(m_hWnd, 34 /*DWMWA_BORDER_COLOR*/, &border, sizeof(border));
}

// fonts, sizes and places for the monitor the dialog is on; again when it moves to one with another scale
void CGroupName::PlaceForScale()
{
	m_titleFont.DeleteObject();
	m_bodyFont.DeleteObject();
	static CString display = FontInstalled(_T("Segoe UI Variable Display")) ? _T("Segoe UI Variable Display") : _T("Segoe UI");
	static CString text = FontInstalled(_T("Segoe UI Variable Text")) ? _T("Segoe UI Variable Text") : _T("Segoe UI");
	m_titleFont.CreateFont(-Scale(20), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, display);
	m_bodyFont.CreateFont(-Scale(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, text);

	int width = Scale(440);
	int pad = Scale(24);
	m_titleRect.SetRect(pad, pad, width - pad, pad + Scale(28));
	int y = m_titleRect.bottom + Scale(8);
	if (m_hint.IsEmpty() == FALSE)
	{
		m_hintRect.SetRect(pad, y, width - pad, y + Scale(20));
		y = m_hintRect.bottom + Scale(8);
	}
	y += Scale(8);
	m_fieldRect.SetRect(pad, y, width - pad, y + Scale(32));
	m_footerTop = m_fieldRect.bottom + pad;
	int height = m_footerTop + Scale(80);

	SetWindowPos(NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

	if (CWnd* edit = GetDlgItem(IDC_NAME))
	{
		edit->SetFont(&m_bodyFont);
		int editHeight = Scale(20);
		edit->SetWindowPos(NULL, m_fieldRect.left + Scale(11), m_fieldRect.CenterPoint().y - editHeight / 2,
			m_fieldRect.Width() - Scale(22), editHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	int buttonWidth = (width - pad * 2 - Scale(8)) / 2;
	int buttonTop = m_footerTop + (height - m_footerTop - Scale(32)) / 2;
	int x = pad;
	for (int id : { IDOK, IDCANCEL })
	{
		if (CWnd* button = GetDlgItem(id))
		{
			button->SetFont(&m_bodyFont);
			button->SetWindowPos(NULL, x, buttonTop, buttonWidth, Scale(32), SWP_NOZORDER | SWP_NOACTIVATE);
			x += buttonWidth + Scale(8);
		}
	}

	FluentDialog::UseContentDialog(m_hWnd, m_footerTop);
	Invalidate();
}

LRESULT CGroupName::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	// keep the place Windows suggests on the new monitor, with this dialog's own size for its scale
	const RECT* suggested = (const RECT*)lParam;
	SetWindowPos(NULL, suggested->left, suggested->top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	PlaceForScale();
	return 0;
}

void CGroupName::OnPaint()
{
	CPaintDC dc(this);
	if (m_footerTop == 0)
	{
		return;
	}

	FluentDialog::Palette card = FluentDialog::ColorsFor(m_hWnd);
	FluentDialog::Palette window = FluentDialog::ColorsFor(NULL);

	CRect client;
	GetClientRect(client);
	dc.FillSolidRect(CRect(client.left, m_footerTop, client.right, client.bottom), window.ground);

	dc.SetBkMode(TRANSPARENT);
	CFont* oldFont = dc.SelectObject(&m_titleFont);
	dc.SetTextColor(card.ink);
	dc.DrawText(m_heading, m_titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
	if (m_hint.IsEmpty() == FALSE)
	{
		dc.SelectObject(&m_bodyFont);
		dc.SetTextColor(card.dim);
		dc.DrawText(m_hint, m_hintRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
	dc.SelectObject(oldFont);

	// Windows 11 text box: a rounded field, a quiet edge, an accent line along the bottom while typing
	FluentDialog::FillRounded(dc.m_hDC, m_fieldRect, Scale(4), card.field);
	FluentDialog::OutlineRounded(dc.m_hDC, m_fieldRect, Scale(4), card.line);
	CWnd* edit = GetDlgItem(IDC_NAME);
	if (edit != NULL && GetFocus() == edit)
	{
		CRect bar(m_fieldRect.left + Scale(2), m_fieldRect.bottom - Scale(2), m_fieldRect.right - Scale(2), m_fieldRect.bottom);
		dc.FillSolidRect(bar, card.accent);
	}
}

void CGroupName::OnNameFocus()
{
	InvalidateRect(m_fieldRect, FALSE);
}

void CGroupName::OnLButtonDown(UINT flags, CPoint point)
{
	// the whole box is the text field, not just the edit inside it
	if (m_fieldRect.PtInRect(point))
	{
		GetDlgItem(IDC_NAME)->SetFocus();
	}
	CDialog::OnLButtonDown(flags, point);
}

HITTEST_RET CGroupName::OnNcHitTest(CPoint point)
{
	// with no title bar, the title area moves the window
	HITTEST_RET hit = CDialog::OnNcHitTest(point);
	CPoint client(point);
	ScreenToClient(&client);
	if (hit == HTCLIENT && client.y < m_fieldRect.top - Scale(4))
	{
		return HTCAPTION;
	}
	return hit;
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
