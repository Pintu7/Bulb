// Draws known text into a CF_DIB-style image and checks ImageOcr reads it back. Build and run: tests\run_tests.bat
#include <windows.h>
#include "..\src\ImageOcr.h"
#include <cstdio>
#include <vector>

#pragma comment(lib, "gdi32")
#pragma comment(lib, "user32")

static std::vector<unsigned char> DrawTextDib(const wchar_t* text)
{
	const int width = 600, height = 120;

	BITMAPINFO info = {};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = width;
	info.bmiHeader.biHeight = height; // bottom-up, like the clipboard
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;

	void* bits = nullptr;
	HDC dc = CreateCompatibleDC(nullptr);
	HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
	HGDIOBJ oldBitmap = SelectObject(dc, bitmap);

	RECT rect = { 0, 0, width, height };
	FillRect(dc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	HFONT font = CreateFontW(-40, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
	HGDIOBJ oldFont = SelectObject(dc, font);
	SetTextColor(dc, RGB(0, 0, 0));
	SetBkMode(dc, TRANSPARENT);
	DrawTextW(dc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	GdiFlush();

	std::vector<unsigned char> dib(sizeof(BITMAPINFOHEADER) + width * height * 4);
	memcpy(dib.data(), &info.bmiHeader, sizeof(BITMAPINFOHEADER));
	memcpy(dib.data() + sizeof(BITMAPINFOHEADER), bits, width * height * 4);

	SelectObject(dc, oldFont);
	SelectObject(dc, oldBitmap);
	DeleteObject(font);
	DeleteObject(bitmap);
	DeleteDC(dc);
	return dib;
}

int main()
{
	int failures = 0;

	std::wstring text = ImageOcr::Recognize(DrawTextDib(L"Bulb OCR test 2026"));
	if (text.find(L"Bulb") == std::wstring::npos || text.find(L"2026") == std::wstring::npos)
	{
		failures++;
		wprintf(L"FAIL: read \"%ls\"\n", text.c_str());
	}

	// an encoded image file (here a .bmp; the clipboard's "PNG" format goes the same way)
	std::vector<unsigned char> dib = DrawTextDib(L"Encoded image 4821");
	BITMAPFILEHEADER file = {};
	file.bfType = 0x4D42;
	file.bfSize = (DWORD)(sizeof(file) + dib.size());
	file.bfOffBits = (DWORD)(sizeof(file) + sizeof(BITMAPINFOHEADER));
	std::vector<unsigned char> bmp((unsigned char*)&file, (unsigned char*)&file + sizeof(file));
	bmp.insert(bmp.end(), dib.begin(), dib.end());
	std::wstring encoded = ImageOcr::Recognize(bmp, false);
	if (encoded.find(L"4821") == std::wstring::npos)
	{
		failures++;
		wprintf(L"FAIL: encoded image read \"%ls\"\n", encoded.c_str());
	}

	if (ImageOcr::Recognize({ 1, 2, 3 }).empty() == false || ImageOcr::Recognize({ 1, 2, 3 }, false).empty() == false)
	{
		failures++;
		wprintf(L"FAIL: garbage bytes should give no text\n");
	}

	if (failures == 0)
	{
		wprintf(L"ImageOcr: all checks pass (read \"%ls\")\n", text.c_str());
	}
	return failures == 0 ? 0 : 1;
}
