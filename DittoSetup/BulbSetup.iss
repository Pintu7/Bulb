; Installer for Bulb, the fork of Ditto. Kept apart from Ditto on purpose: own AppId, folder,
; registry key and startup entry, no .dto association and no firewall rule, so both can be installed side by side.
#define MyAppName      "Bulb"
#define MyAppExe       "Bulb.exe"
#define MyAppVersion   GetFileVersion("..\Release64\Bulb.exe")
; shortcut: pinned to the installed toolset's redist version, update it when Visual Studio updates the redist
#define Redist         "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Redist\MSVC\14.51.36231\x64"

[Setup]
AppId={{C3EACA63-16E3-4856-BFD5-50B3DD312F24}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher=Pintu7
WizardStyle=modern
UninstallDisplayIcon={app}\{#MyAppExe}
SetupIconFile=..\res\Bulb.ico
UninstallDisplayName={#MyAppName}
VersionInfoDescription={#MyAppName} installer
VersionInfoProductName={#MyAppName}
OutputDir=Output
OutputBaseFilename=BulbSetup
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableReadyPage=yes
DirExistsWarning=no
CloseApplications=yes
MinVersion=10.0

[Languages]
Name: Italiano; MessagesFile: compiler:Languages\Italian.isl
Name: English; MessagesFile: compiler:Default.isl

[CustomMessages]
Italiano.RunOnStartup=Avvia Bulb all'accesso a Windows
Italiano.LaunchApp=Avvia Bulb
English.RunOnStartup=Start Bulb when I sign in to Windows
English.LaunchApp=Launch Bulb

[Tasks]
Name: RunAtStartup; Description: {cm:RunOnStartup}

[InstallDelete]
; early builds shipped the add-in under its Ditto name; left there it loads twice next to BulbUtil.dll
Type: files; Name: "{app}\Addins\DittoUtil.dll"

[Files]
Source: ..\Release64\Bulb.exe; DestDir: {app}; Flags: ignoreversion
Source: ..\Release64\ICU_Loader.dll; DestDir: {app}; Flags: ignoreversion skipifsourcedoesntexist
Source: ..\Release64\Addins\DittoUtil.dll; DestDir: {app}\Addins; DestName: BulbUtil.dll; Flags: ignoreversion
; the runtime Bulb was built against, from Visual Studio's redistributable folder (not whatever System32 has)
Source: {#Redist}\Microsoft.VC145.CRT\vcruntime140.dll; DestDir: {app}; Flags: ignoreversion
Source: {#Redist}\Microsoft.VC145.CRT\vcruntime140_1.dll; DestDir: {app}; Flags: ignoreversion
Source: {#Redist}\Microsoft.VC145.CRT\msvcp140.dll; DestDir: {app}; Flags: ignoreversion
Source: {#Redist}\Microsoft.VC145.MFC\mfc140u.dll; DestDir: {app}; Flags: ignoreversion
Source: ..\Debug\Language\*; DestDir: {app}\Language
Source: ..\Debug\Themes\*; DestDir: {app}\Themes

[Icons]
Name: {group}\{#MyAppName}; Filename: {app}\{#MyAppExe}
Name: {group}\{cm:UninstallProgram,{#MyAppName}}; Filename: {uninstallexe}

[Registry]
Root: HKCU; Subkey: Software\Bulb; Flags: uninsdeletekey
Root: HKCU; Subkey: SOFTWARE\Microsoft\Windows\CurrentVersion\Run; ValueType: string; ValueName: Bulb; ValueData: """{app}\{#MyAppExe}"""; Flags: uninsdeletevalue; Tasks: RunAtStartup

[Run]
Filename: {app}\{#MyAppExe}; Description: {cm:LaunchApp}; Flags: nowait postinstall skipifsilent

[UninstallRun]
; only if Bulb did not close by itself (see InitializeUninstall)
Filename: taskkill; Parameters: "/f /im {#MyAppExe}"; Flags: runhidden; RunOnceId: StopBulb

[Code]
const
  WM_CLOSE = $0010;

// asks the running Bulb to close the way its tray Exit does, so the database is closed cleanly
function InitializeUninstall(): Boolean;
var
  Wnd: HWND;
  Waited: Integer;
begin
  Wnd := FindWindowByClassName('Bulb');
  if Wnd <> 0 then
  begin
    PostMessage(Wnd, WM_CLOSE, 0, 0);
    Waited := 0;
    while (FindWindowByClassName('Bulb') <> 0) and (Waited < 5000) do
    begin
      Sleep(100);
      Waited := Waited + 100;
    end;
  end;
  Result := True;
end;
