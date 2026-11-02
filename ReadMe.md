# Bulb – a fork of Ditto

**Bulb is a personal fork of [Ditto](https://github.com/sabrogden/Ditto), the clipboard manager for Windows by Scott Brogden and its contributors.** Everything that makes it work, from saving every clip to searching and pasting them, is Ditto's work. Bulb only adds and changes a few things on top. If you want the original, maintained program, use [Ditto](https://github.com/sabrogden/Ditto) and consider [supporting it](https://github.com/sabrogden/Ditto#readme).

Bulb is made by Andrea Rinaldi ([Pintu7](https://github.com/Pintu7)) and is not affiliated with or endorsed by the Ditto project. Found a bug or want something? [Open an issue](https://github.com/Pintu7/Bulb/issues).

## Why this fork

I use Ditto every day and wanted a version that fits the way I work:

- a clipboard manager that looks at home on Windows 11;
- tags instead of groups, so a clip can belong to more than one place;
- a way to type the clipboard into windows that refuse a paste, such as a remote session;
- clips that are never deleted automatically because of what they contain.

These changes are personal choices rather than fixes Ditto needs, so they live in a fork instead of pull requests.

## What is different from Ditto

- **Windows 11 look**: rounded windows, Segoe UI Variable and Fluent icons, light and dark themes, and settings laid out like the Windows 11 Settings app.
- **Tags**: tag clips with Ctrl+T, filter the list with the tag bar or `#tag` in the search, and convert existing groups to tags.
- **Go to Entry shortcut**: Ctrl+Enter on a search result shows the clip in the full history, with what you copied before and after it.
- **Type Clipboard**: Ctrl+Alt+V types the clipboard text as key presses, for TeamViewer or Remote Desktop sessions that block paste. It types at most 100 characters, 50 ms apart, and Esc stops it; the shortcut and both values can be changed in Options > Keyboard. **Type Clip** in a clip's right-click menu types that clip the same way.
- **Text from images**: copy the text of an image clip, recognised with Windows' built-in OCR.
- **Paste queue**: paste several clips one after another.
- **Safer defaults**: clips are never deleted because their content looks sensitive, and the network receive port never opens with the default password.
- **Its own name and folder**: Bulb installs and stores its data apart from Ditto, so both can be installed side by side.
- **Check for updates**: a button in Options > About asks GitHub for the newest release and opens its page. Bulb never checks by itself.

Like Ditto, Bulb is local first: no login, no cloud, no telemetry.

## Building

To install Bulb, download the installer from [Releases](https://github.com/Pintu7/Bulb/releases). To build it from source on Windows:

1. Install Visual Studio 2026 (or its Build Tools) with the C++ desktop workload and MFC.
2. Build: `MSBuild CP_Main_10.sln /p:Configuration=Release /p:Platform=x64`. The program is `Release64\Bulb.exe`.
3. Installer (optional): compile `DittoSetup\BulbSetup.iss` with [Inno Setup 6](https://jrsoftware.org/isinfo.php).
4. Tests: run `tests\run_tests.bat` from a Visual Studio developer prompt.

## License

Bulb is distributed under the GNU General Public License v3, the same license as Ditto: see [LICENSE](LICENSE). The original copyright and credits stay with Ditto's authors.
