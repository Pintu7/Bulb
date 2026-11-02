#pragma once

#include <vector>

// Windows 11 look for a classic dialog or property sheet, in Bulb's light or dark colors: Segoe UI Variable,
// themed fields and buttons, check boxes / radios / group boxes / tabs drawn so they stay readable on a dark ground.
// Pages created later (property sheet tabs) are themed when they are first shown.
namespace FluentDialog
{
	// Bulb's one set of interface colors, light or dark; menus, navigation and pickers draw from it too
	struct Palette
	{
		bool dark;
		COLORREF ground, field, ink, dim, line, selected, accent;
		COLORREF disabled;             // text of what can't be used
		COLORREF menu, menuHover;      // a menu or flyout surface, and an item under the mouse on it
		COLORREF navHover, navSelected; // a navigation row on the window ground: under the mouse, open
	};

	void Apply(HWND dialog);

	// call once on the UI thread: from then on every dialog and message box it opens gets Apply when first shown
	void ThemeAllDialogs();

	// true when Bulb draws with its dark colors
	bool IsDark();

	// 'a' moved 'percentOfB' percent of the way to 'b'
	COLORREF Mix(COLORREF a, COLORREF b, int percentOfB);
	// perceived brightness, 0 (black) to 255 (white)
	int Luma(COLORREF color);

	// the colors for what is drawn on 'surface': the window ground, or the card of a settings page
	Palette ColorsFor(HWND surface);

	// Windows 11 Settings layout for a property sheet: every page sits at 'pageRect' (sheet client coordinates)
	// on a raised card, and its check boxes become switches. 'pageRect' must stay valid until the sheet passes NULL
	// (in its OnDestroy).
	void UseSettingsPages(HWND sheet, const RECT* pageRect);

	// Windows 11 content dialog: the dialog is a raised card and, from 'footerTop' (client y) down, a footer band
	// in the window ground holds its buttons. Call from OnInitDialog; the dialog paints the band itself.
	void UseContentDialog(HWND dialog, int footerTop);

	// Windows 11 Settings page layout: every setting is a card row with its name (and a description) on the
	// left and its controls on the right; sections get a heading; the page scrolls when it is taller than its
	// window. Controls are the page's own (moved, never recreated), titles come from their translated text.
	// Build it at the end of the page's OnInitDialog; it lays out again whenever the page is resized.
	class SettingsLayout
	{
	public:
		enum { Keep = 0, Fill = -1, Below = -2 }; // control width: as designed, the room left, full width under the title
		struct Item { int id; int width; };

		// a heading; from a group box's text (the box itself is hidden) or from a literal
		SettingsLayout& Section(int groupBoxId);
		SettingsLayout& SectionText(const CString& title);
		// a row named by a label or check box (a label is hidden; a check box listed in 'controls' becomes the switch);
		// "Name (more words)" splits into a title and a description
		SettingsLayout& Row(int titleId, const std::vector<Item>& controls, int descriptionId = 0);
		SettingsLayout& RowText(const CString& title, const std::vector<Item>& controls, const CString& description = CString());
		SettingsLayout& Describe(const CString& description); // a description for the row just added
		SettingsLayout& Hide(const std::vector<int>& ids);

		void AttachTo(HWND page);

		struct Entry
		{
			bool section = false;
			int titleId = 0;
			CString title;
			CString description;
			int descriptionId = 0;
			std::vector<Item> controls;
		};
	private:
		std::vector<Entry> m_entries;
		std::vector<int> m_hidden;
	};

	bool HasSettingsLayout(HWND page);

	// a static control that opens a link: its text in the accent color, as Windows 11 writes links
	void MarkLink(HWND link);

	// antialiased rounded rectangles, as the menus and the navigation draw them
	void FillRounded(HDC dc, const RECT& rect, int radius, COLORREF color);
	void OutlineRounded(HDC dc, const RECT& rect, int radius, COLORREF color);
}
