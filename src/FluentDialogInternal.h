#pragma once

// Shared only by FluentDialog.cpp (themed dialogs and controls) and FluentSettings.cpp (the settings page layout).
#include "FluentDialog.h"

namespace FluentInternal
{
	using FluentDialog::Palette;
	using FluentDialog::Mix;
	using FluentDialog::Luma;

	inline const UINT_PTR DialogSubclassId = 0xF1E8;
	// window property names as atoms registered once: a property left on a control then costs nothing when it goes
	inline const LPCTSTR NoLabelProp = MAKEINTATOM(GlobalAddAtom(_T("BulbFluentNoLabel"))); // a switch whose name is the row's title
	inline const LPCTSTR LayoutProp = MAKEINTATOM(GlobalAddAtom(_T("BulbSettingsLayout")));
	inline const LPCTSTR DimProp = MAKEINTATOM(GlobalAddAtom(_T("BulbFluentDim")));

	// FluentDialog.cpp
	Palette Colors();
	Palette ColorsOn(HWND surface);
	HBRUSH Brush(COLORREF color);
	int Scale(HWND hwnd, int value);
	LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);

	// FluentSettings.cpp: what DialogProc asks of a page that has a settings layout
	void LayoutPage(HWND page);
	void ScrollPage(HWND page, int position);
	void PaintRows(HWND page, HDC dc);
	bool InRow(HWND page, HWND control);
	int PageScroll(HWND page);
	int PageScrollMax(HWND page);
	void FreeLayout(HWND page);
}