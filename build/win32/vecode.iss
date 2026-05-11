; NVECode — Inno Setup installer script
;
; Build the Windows binary first:
;   "C:\Program Files\Git\bin\bash.exe" ./dev/build.sh
;
; Then compile the installer:
;   "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" build\win32\vecode.iss
;
; Output: vecode-${VERSION}-${ARCH}-setup.exe
;
; No brand string is hard-coded below — everything is read from product.json via
; preprocessor directives, so the same script works for an Acme-Code re-brand
; with no edits.

#define ProductJson "..\..\vscode\product.json"

#define AppName         GetStringFileInfo(GetEnv("NVECODE_PRODUCT_NAME"),     "NVECode")
#define AppNameLong     GetStringFileInfo(GetEnv("NVECODE_PRODUCT_LONG"),     "Neofilisoft Visual Editor Code")
#define BinaryName      GetStringFileInfo(GetEnv("NVECODE_BINARY_NAME"),      "vecode")
#define Publisher       GetStringFileInfo(GetEnv("NVECODE_PUBLISHER"),        "Neofilisoft")
#define AppUrl          GetStringFileInfo(GetEnv("NVECODE_APP_URL"),          "https://github.com/Neofilisoft/nvecode")
#define AppId           GetStringFileInfo(GetEnv("NVECODE_APP_ID"),           "{{79269904-5A90-4033-AAB1-D1D04EC743CA}")
#define AppIdUser       GetStringFileInfo(GetEnv("NVECODE_APP_ID_USER"),      "{{4C628DBB-2CC0-4372-AB4B-BCDAF8198723}")
#define UrlProtocol     GetStringFileInfo(GetEnv("NVECODE_URL_PROTOCOL"),     "nvecode")
#define DataFolderName  GetStringFileInfo(GetEnv("NVECODE_DATA_FOLDER_NAME"), ".nvecode")

; SourceDir points at the built artifact tree, e.g. VSCode-win32-x64
#ifndef SourceDir
  #define SourceDir "..\..\VSCode-win32-x64"
#endif
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef AppArch
  #define AppArch "x64"
#endif
#ifndef UserMode
  #define UserMode "no"
#endif

[Setup]
AppId={#UserMode == "yes" ? AppIdUser : AppId}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#Publisher}
AppPublisherURL={#AppUrl}
AppSupportURL={#AppUrl}/issues
AppUpdatesURL={#AppUrl}/releases
DefaultDirName={#UserMode == "yes" ? "{userpf}\" + AppName : "{autopf}\" + AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
DisableReadyPage=yes
DisableWelcomePage=yes
OutputBaseFilename={#BinaryName}-{#AppVersion}-{#AppArch}-setup
Compression=lzma2/ultra64
SolidCompression=yes
SetupIconFile={#SourceDir}\resources\app\resources\win32\code.ico
UninstallDisplayIcon={app}\{#BinaryName}.exe
UninstallDisplayName={#AppName}
LicenseFile={#SourceDir}\resources\app\LICENSE.txt
PrivilegesRequired={#UserMode == "yes" ? "lowest" : "admin"}
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesInstallIn64BitMode={#AppArch == "x64" ? "x64compatible" : (AppArch == "arm64" ? "arm64" : "")}
ArchitecturesAllowed={#AppArch == "x64" ? "x64compatible" : (AppArch == "arm64" ? "arm64" : "x86")}
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon";          Description: "Create a &desktop icon";                          GroupDescription: "Additional icons:";  Flags: unchecked
Name: "quicklaunchicon";      Description: "Create a &Quick Launch icon";                     GroupDescription: "Additional icons:";  Flags: unchecked; OnlyBelowVersion: 0,6.1
Name: "addcontextmenufiles";  Description: "Add ""Open with {#AppName}"" to file context menu";     GroupDescription: "Other:";              Flags: unchecked
Name: "addcontextmenufolders";Description: "Add ""Open with {#AppName}"" to directory context menu";GroupDescription: "Other:";              Flags: unchecked
Name: "associatewithfiles";   Description: "Register {#AppName} as an editor for supported file types"; GroupDescription: "Other:";    Flags: unchecked
Name: "addtopath";            Description: "Add to PATH (requires shell restart)";            GroupDescription: "Other:";              Flags: checkedonce

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Excludes: "*.log"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\{#AppName}";          Filename: "{app}\{#BinaryName}.exe"
Name: "{autodesktop}\{#AppName}";    Filename: "{app}\{#BinaryName}.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#AppName}"; Filename: "{app}\{#BinaryName}.exe"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#BinaryName}.exe"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{%USERPROFILE}\{#DataFolderName}"
Type: filesandordirs; Name: "{app}"

[Registry]
; URL protocol handler for vecode://
Root: HKA; Subkey: "Software\Classes\{#UrlProtocol}"; ValueType: string; ValueName: ""; ValueData: "URL:{#AppName} Protocol"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#UrlProtocol}"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\{#UrlProtocol}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#BinaryName}.exe,0"
Root: HKA; Subkey: "Software\Classes\{#UrlProtocol}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#BinaryName}.exe"" --open-url -- ""%1"""

; PATH entry (added on demand)
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}\bin"; Check: NeedsAddPath('{app}\bin'); Tasks: addtopath; Flags: preservestringtype

[Code]
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
    'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  Result := Pos(';' + Param + ';', ';' + OrigPath + ';') = 0;
end;
