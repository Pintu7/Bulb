@echo off
rem Builds and runs Bulb's standalone checks with the Visual Studio compiler.
setlocal
rem a CL=/MP left from a parallel Bulb build would break the files that use stdafx.h's #import
set CL=
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul || exit /b 1
set OUT=%~dp0bin
if not exist "%OUT%" mkdir "%OUT%"
set FAILED=0

cl /nologo /std:c++20 /EHsc /W3 /utf-8 /Fe"%OUT%\ImageOcrTest.exe" /Fo"%OUT%\\" "%~dp0ImageOcrTest.cpp" "%~dp0..\src\ImageOcr.cpp" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
"%OUT%\ImageOcrTest.exe" || set FAILED=1

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /Fe"%OUT%\TextTransformsTest.exe" /Fo"%OUT%\\" "%~dp0TextTransformsTest.cpp" "%~dp0..\src\TextTransforms.cpp" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
"%OUT%\TextTransformsTest.exe" || set FAILED=1

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /Fe"%OUT%\BulbFolderTest.exe" /Fo"%OUT%\\" "%~dp0BulbFolderTest.cpp" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
"%OUT%\BulbFolderTest.exe" || set FAILED=1

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /Fe"%OUT%\PasteQueueKeysTest.exe" /Fo"%OUT%\\" "%~dp0PasteQueueKeysTest.cpp" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
"%OUT%\PasteQueueKeysTest.exe" || set FAILED=1

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /Fe"%OUT%\SearchTagsTest.exe" /Fo"%OUT%\\" "%~dp0SearchTagsTest.cpp" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
"%OUT%\SearchTagsTest.exe" || set FAILED=1

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /Fe"%OUT%\AutoTypeTest.exe" /Fo"%OUT%\\" "%~dp0AutoTypeTest.cpp" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
"%OUT%\AutoTypeTest.exe" || set FAILED=1

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /Fe"%OUT%\UpdateCheckTest.exe" /Fo"%OUT%\\" "%~dp0UpdateCheckTest.cpp" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
"%OUT%\UpdateCheckTest.exe" || set FAILED=1

rem SQLite is large and never changes here: compiled once and kept
if not exist "%OUT%\sqlite3mc.obj" cl /nologo /c /O2 /w /Fo"%OUT%\sqlite3mc.obj" "%~dp0..\src\sqlite\sqlite3mc_amalgamation.c" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
cl /nologo /std:c++17 /EHsc /W3 /utf-8 /DUNICODE /D_UNICODE /I"%~dp0..\src" /Fe"%OUT%\DittoImportTest.exe" /Fo"%OUT%\\" "%~dp0DittoImportTest.cpp" "%~dp0..\src\DittoImport.cpp" "%~dp0..\src\sqlite\CppSQLite3.cpp" "%OUT%\sqlite3mc.obj" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
"%OUT%\DittoImportTest.exe" || set FAILED=1

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /Fe"%OUT%\ZipBoundsTest.exe" /Fo"%OUT%\\" "%~dp0ZipBoundsTest.cpp" "%~dp0..\src\zlib\adler32.c" "%~dp0..\src\zlib\compress.c" "%~dp0..\src\zlib\crc32.c" "%~dp0..\src\zlib\deflate.c" "%~dp0..\src\zlib\inffast.c" "%~dp0..\src\zlib\inflate.c" "%~dp0..\src\zlib\inftrees.c" "%~dp0..\src\zlib\trees.c" "%~dp0..\src\zlib\uncompr.c" "%~dp0..\src\zlib\zutil.c" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
"%OUT%\ZipBoundsTest.exe" || set FAILED=1

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /DUNICODE /D_UNICODE /I"%~dp0..\src" /Fe"%OUT%\HtmlPreviewOfflineTest.exe" /Fo"%OUT%\\" "%~dp0HtmlPreviewOfflineTest.cpp" "%~dp0..\src\SimpleBrowser.cpp" >"%OUT%\build.log" || (type "%OUT%\build.log" & exit /b 1)
"%OUT%\HtmlPreviewOfflineTest.exe" || set FAILED=1

rem Bulb has no network code of its own: the built program may use WinHTTP for Check for updates and nothing else
if exist "%~dp0..\Release64\Bulb.exe" (
	dumpbin /nologo /imports "%~dp0..\Release64\Bulb.exe" | findstr /i /r "ws2_32\.dll wsock32\.dll mswsock\.dll urlmon\.dll wininet\.dll InternetOpen InternetConnect InternetReadFile URLDownloadTo" >nul && (echo FAIL: Bulb.exe imports a network library & set FAILED=1) || echo NoNetworkImports: 1 check passed
)

exit /b %FAILED%

