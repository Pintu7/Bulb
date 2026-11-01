#pragma once

// Windows 11 style popup menus: taller owner-drawn rows, icons, rounded hover and rounded corners, in the theme's colors.
namespace FluentMenu
{
	// Same contract as ::TrackPopupMenu (flags, return value, WM_COMMAND to owner); the menu is left as it was found.
	UINT Track(HMENU menu, UINT flags, int x, int y, HWND owner);
}
