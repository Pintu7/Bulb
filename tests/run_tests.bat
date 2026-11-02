@echo off
rem Builds and runs Bulb's standalone checks with the Visual Studio compiler.
setlocal
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

exit /b %FAILED%

