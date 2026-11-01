// OptionsCopyBuffers.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "OptionsCopyBuffers.h"
#include "FluentDialog.h"


// COptionsCopyBuffers dialog

IMPLEMENT_DYNCREATE(COptionsCopyBuffers, CPropertyPage)

COptionsCopyBuffers::COptionsCopyBuffers() : CPropertyPage(COptionsCopyBuffers::IDD)
{
	m_csTitle = theApp.m_Language.GetString("CopyBuffers", "Copy Buffers");
	m_psp.pszTitle = m_csTitle;
	m_psp.dwFlags |= PSP_USETITLE; 
}

COptionsCopyBuffers::~COptionsCopyBuffers()
{
}

void COptionsCopyBuffers::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COPY_1, m_CopyBuffer1);
	DDX_Control(pDX, IDC_PASTE_1, m_PasteBuffer1);
	DDX_Control(pDX, IDC_CUT_1, m_CutBuffer1);
	DDX_Control(pDX, IDC_COPY_2, m_CopyBuffer2);
	DDX_Control(pDX, IDC_PASTE_2, m_PasteBuffer2);
	DDX_Control(pDX, IDC_CUT_2, m_CutBuffer2);
	DDX_Control(pDX, IDC_COPY_3, m_CopyBuffer3);
	DDX_Control(pDX, IDC_PASTE_3, m_PasteBuffer3);
	DDX_Control(pDX, IDC_CUT_3, m_CutBuffer3);

	DDX_Control(pDX, IDC_COPY_4, m_CopyBuffer4);
	DDX_Control(pDX, IDC_PASTE_4, m_PasteBuffer4);
	DDX_Control(pDX, IDC_CUT_4, m_CutBuffer4);

	DDX_Control(pDX, IDC_COPY_5, m_CopyBuffer5);
	DDX_Control(pDX, IDC_PASTE_5, m_PasteBuffer5);
	DDX_Control(pDX, IDC_CUT_5, m_CutBuffer5);
}


BEGIN_MESSAGE_MAP(COptionsCopyBuffers, CPropertyPage)
END_MESSAGE_MAP()

BOOL COptionsCopyBuffers::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CCopyBufferItem Item;
	
	CGetSetOptions::GetCopyBufferItem(0, Item);
	theApp.m_pCopyBuffer1->CopyToCtrl(m_CopyBuffer1, m_hWnd, IDC_WIN_COPY_1);
	theApp.m_pPasteBuffer1->CopyToCtrl(m_PasteBuffer1, m_hWnd, IDC_WIN_PASTE_1);
	theApp.m_pCutBuffer1->CopyToCtrl(m_CutBuffer1, m_hWnd, IDC_WIN_CUT_1);
	CheckDlgButton(IDC_PLAY_SOUND_1, Item.m_bPlaySoundOnCopy);

	CGetSetOptions::GetCopyBufferItem(1, Item);
	theApp.m_pCopyBuffer2->CopyToCtrl(m_CopyBuffer2, m_hWnd, IDC_WIN_COPY_2);
	theApp.m_pPasteBuffer2->CopyToCtrl(m_PasteBuffer2, m_hWnd, IDC_WIN_PASTE_2);
	theApp.m_pCutBuffer2->CopyToCtrl(m_CutBuffer2, m_hWnd, IDC_WIN_CUT_2);
	CheckDlgButton(IDC_PLAY_SOUND_2, Item.m_bPlaySoundOnCopy);

	CGetSetOptions::GetCopyBufferItem(2, Item);
	theApp.m_pCopyBuffer3->CopyToCtrl(m_CopyBuffer3, m_hWnd, IDC_WIN_COPY_3);
	theApp.m_pPasteBuffer3->CopyToCtrl(m_PasteBuffer3, m_hWnd, IDC_WIN_PASTE_3);
	theApp.m_pCutBuffer3->CopyToCtrl(m_CutBuffer3, m_hWnd, IDC_WIN_CUT_3);
	CheckDlgButton(IDC_PLAY_SOUND_3, Item.m_bPlaySoundOnCopy);

	CGetSetOptions::GetCopyBufferItem(3, Item);
	theApp.m_pCopyBuffer4->CopyToCtrl(m_CopyBuffer4, m_hWnd, IDC_WIN_COPY_4);
	theApp.m_pPasteBuffer4->CopyToCtrl(m_PasteBuffer4, m_hWnd, IDC_WIN_PASTE_4);
	theApp.m_pCutBuffer4->CopyToCtrl(m_CutBuffer4, m_hWnd, IDC_WIN_CUT_4);
	CheckDlgButton(IDC_PLAY_SOUND_4, Item.m_bPlaySoundOnCopy);

	CGetSetOptions::GetCopyBufferItem(4, Item);
	theApp.m_pCopyBuffer5->CopyToCtrl(m_CopyBuffer5, m_hWnd, IDC_WIN_COPY_5);
	theApp.m_pPasteBuffer5->CopyToCtrl(m_PasteBuffer5, m_hWnd, IDC_WIN_PASTE_5);
	theApp.m_pCutBuffer5->CopyToCtrl(m_CutBuffer5, m_hWnd, IDC_WIN_CUT_5);
	CheckDlgButton(IDC_PLAY_SOUND_5, Item.m_bPlaySoundOnCopy);

	theApp.m_Language.UpdateOptionCopyBuffers(this);

	// Windows 11 Settings rows, one section per buffer
	using L = FluentDialog::SettingsLayout;
	const int key = 150, win = 84;
	const int groups[] = { IDC_BUFFER_GROUP_1, IDC_BUFFER_GROUP_2, IDC_BUFFER_GROUP_3, IDC_BUFFER_GROUP_4, IDC_BUFFER_GROUP_5 };
	const int copyLabels[] = { IDC_STATIC_COPY_1, IDC_STATIC_COPY_2, IDC_STATIC_COPY_3, IDC_STATIC_COPY_4, IDC_STATIC_COPY_5 };
	const int copies[] = { IDC_COPY_1, IDC_COPY_2, IDC_COPY_3, IDC_COPY_4, IDC_COPY_5 };
	const int copyWins[] = { IDC_WIN_COPY_1, IDC_WIN_COPY_2, IDC_WIN_COPY_3, IDC_WIN_COPY_4, IDC_WIN_COPY_5 };
	const int pasteLabels[] = { IDC_STATIC_PASTE_1, IDC_STATIC_PASTE_2, IDC_STATIC_PASTE_3, IDC_STATIC_PASTE_4, IDC_STATIC_PASTE_5 };
	const int pastes[] = { IDC_PASTE_1, IDC_PASTE_2, IDC_PASTE_3, IDC_PASTE_4, IDC_PASTE_5 };
	const int pasteWins[] = { IDC_WIN_PASTE_1, IDC_WIN_PASTE_2, IDC_WIN_PASTE_3, IDC_WIN_PASTE_4, IDC_WIN_PASTE_5 };
	const int cutLabels[] = { IDC_STATIC_CUT_1, IDC_STATIC_CUT_2, IDC_STATIC_CUT_3, IDC_STATIC_CUT_4, IDC_STATIC_CUT_5 };
	const int cuts[] = { IDC_CUT_1, IDC_CUT_2, IDC_CUT_3, IDC_CUT_4, IDC_CUT_5 };
	const int cutWins[] = { IDC_WIN_CUT_1, IDC_WIN_CUT_2, IDC_WIN_CUT_3, IDC_WIN_CUT_4, IDC_WIN_CUT_5 };
	const int sounds[] = { IDC_PLAY_SOUND_1, IDC_PLAY_SOUND_2, IDC_PLAY_SOUND_3, IDC_PLAY_SOUND_4, IDC_PLAY_SOUND_5 };
	L layout;
	for (int i = 0; i < 5; i++)
	{
		layout.Section(groups[i])
			.Row(copyLabels[i], { { copies[i], key }, { copyWins[i], win } })
			.Row(pasteLabels[i], { { pastes[i], key }, { pasteWins[i], win } })
			.Row(cutLabels[i], { { cuts[i], key }, { cutWins[i], win } })
			.Row(sounds[i], { { sounds[i], 48 } });
	}
	layout.AttachTo(m_hWnd);

	return TRUE;
}

BOOL COptionsCopyBuffers::OnApply()
{
	ARRAY keys;
	g_HotKeys.GetKeys(keys); // save old keys just in case new ones are invalid

	theApp.m_pCopyBuffer1->CopyFromCtrl(m_CopyBuffer1, m_hWnd, IDC_WIN_COPY_1);
	theApp.m_pPasteBuffer1->CopyFromCtrl(m_PasteBuffer1, m_hWnd, IDC_WIN_PASTE_1);
	theApp.m_pCutBuffer1->CopyFromCtrl(m_CutBuffer1, m_hWnd, IDC_WIN_CUT_1);

	theApp.m_pCopyBuffer2->CopyFromCtrl(m_CopyBuffer2, m_hWnd, IDC_WIN_COPY_2);
	theApp.m_pPasteBuffer2->CopyFromCtrl(m_PasteBuffer2, m_hWnd, IDC_WIN_PASTE_2);
	theApp.m_pCutBuffer2->CopyFromCtrl(m_CutBuffer2, m_hWnd, IDC_WIN_CUT_2);

	theApp.m_pCopyBuffer3->CopyFromCtrl(m_CopyBuffer3, m_hWnd, IDC_WIN_COPY_3);
	theApp.m_pPasteBuffer3->CopyFromCtrl(m_PasteBuffer3, m_hWnd, IDC_WIN_PASTE_3);
	theApp.m_pCutBuffer3->CopyFromCtrl(m_CutBuffer3, m_hWnd, IDC_WIN_CUT_3);

	theApp.m_pCopyBuffer4->CopyFromCtrl(m_CopyBuffer4, m_hWnd, IDC_WIN_COPY_4);
	theApp.m_pPasteBuffer4->CopyFromCtrl(m_PasteBuffer4, m_hWnd, IDC_WIN_PASTE_4);
	theApp.m_pCutBuffer4->CopyFromCtrl(m_CutBuffer4, m_hWnd, IDC_WIN_CUT_4);

	theApp.m_pCopyBuffer5->CopyFromCtrl(m_CopyBuffer5, m_hWnd, IDC_WIN_COPY_5);
	theApp.m_pPasteBuffer5->CopyFromCtrl(m_PasteBuffer5, m_hWnd, IDC_WIN_PASTE_5);
	theApp.m_pCutBuffer5->CopyFromCtrl(m_CutBuffer5, m_hWnd, IDC_WIN_CUT_5);

	CCopyBufferItem Item;
	CGetSetOptions::GetCopyBufferItem(0, Item);
	Item.m_bPlaySoundOnCopy = IsDlgButtonChecked(IDC_PLAY_SOUND_1);
	CGetSetOptions::SetCopyBufferItem(0, Item);

	CGetSetOptions::GetCopyBufferItem(1, Item);
	Item.m_bPlaySoundOnCopy = IsDlgButtonChecked(IDC_PLAY_SOUND_2);
	CGetSetOptions::SetCopyBufferItem(1, Item);

	CGetSetOptions::GetCopyBufferItem(2, Item);
	Item.m_bPlaySoundOnCopy = IsDlgButtonChecked(IDC_PLAY_SOUND_3);
	CGetSetOptions::SetCopyBufferItem(2, Item);

	CGetSetOptions::GetCopyBufferItem(3, Item);
	Item.m_bPlaySoundOnCopy = IsDlgButtonChecked(IDC_PLAY_SOUND_4);
	CGetSetOptions::SetCopyBufferItem(3, Item);

	CGetSetOptions::GetCopyBufferItem(4, Item);
	Item.m_bPlaySoundOnCopy = IsDlgButtonChecked(IDC_PLAY_SOUND_5);
	CGetSetOptions::SetCopyBufferItem(4, Item);

	INT_PTR x;
	INT_PTR y;
	ARRAY NewKeys;
	g_HotKeys.GetKeys(NewKeys);

	if(g_HotKeys.FindFirstConflict(NewKeys, &x, &y))
	{
		CString str =  g_HotKeys.ElementAt(x)->GetName();
		str += " and ";
		str += g_HotKeys.ElementAt(y)->GetName();
		str += " cannot be the same.";
		MessageBox(str);
		g_HotKeys.SetKeys(keys); // restore the original values
		return FALSE;
	}

	g_HotKeys.SaveAllKeys();
	g_HotKeys.RegisterAll(true);

	return CPropertyPage::OnApply();
}

