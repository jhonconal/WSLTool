; WSL Tool Inno Setup 安装脚本
; 需要 Inno Setup 6.x: https://jrsoftware.org/isinfo.php

#define MyAppName "WSL 管理工具"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "WSLTool"
#define MyAppExeName "WSLTool.exe"
#define MyBuildDir "..\build\release"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\WSLTool
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=..\installer
OutputBaseFilename=WSLTool_v{#MyAppVersion}_Setup
SetupIconFile=..\resources\app_icon.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
MinVersion=6.1.7600
; Windows 7 最低要求
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加任务:"; Flags: unchecked
Name: "quicklaunchicon"; Description: "创建快速启动栏快捷方式"; GroupDescription: "附加任务:"; Flags: unchecked; OnlyBelowVersion: 6.1

[Files]
; 主程序
Source: "{#MyBuildDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; 所有依赖的 DLL 动态库 (windeployqt 部署的文件)
Source: "{#MyBuildDir}\*.dll"; DestDir: "{app}"; Flags: ignoreversion

; Qt 平台插件
Source: "{#MyBuildDir}\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs
Source: "{#MyBuildDir}\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist
Source: "{#MyBuildDir}\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs skipifsourcedoesntexist

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\卸载 {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "立即启动 WSL 管理工具"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[CustomMessages]
chinesesimplified.WelcomeLabel2=本向导将引导您安装 [name/ver]。%n%n%n⚠️ 重要提示：%n本工具涉及 WSL 发行版管理操作，请确保在使用迁移功能前做好数据备份！%n%n建议在继续安装前关闭所有 WSL 实例。

[Code]
// 检查 WSL 是否已安装（提示而不阻止安装）
function InitializeSetup(): Boolean;
begin
  Result := True;
  if not FileExists(ExpandConstant('{sys}\wsl.exe')) then
  begin
    if MsgBox('未检测到 WSL（wsl.exe）。本工具需要 WSL 环境才能使用完整功能。' + #13#10 +
              '是否仍继续安装？', mbConfirmation, MB_YESNO) = IDNO then
      Result := False;
  end;
end;
