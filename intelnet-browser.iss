; 明镜浏览器 Inno Setup 安装脚本
; Inno Setup 7.x
; 打包前确认：
;   1. Release 版 exe 已编译，resources/ 目录完整（llama/ + models/ + Qt 资源）
;   2. installer/vc_redist.x64.exe 已下载（https://aka.ms/vs/17/release/vc_redist.x64.exe）
;   3. BundledVCRedistVersion 与下载的 vc_redist 版本一致
;   4. LICENSE 已换成 GPLv3（10-13 前）

#define MyAppName "明镜浏览器"
#define MyAppEnglishName "IntelNet Browser"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "RUV2005"
#define MyAppURL "https://github.com/RUV2005/intelnet-browser"
#define MyAppExeName "IntelNetBrowser.exe"
; 磁盘目录用英文（llama.cpp / Piper 对中文路径支持有坑），显示名保持中文
#define MyAppDirName "IntelNetBrowser"

; 构建产物根目录（Qt Release 输出 + windeployqt 后的完整目录）
#define BuildRoot "qt-client\build\Release"
; Rust 核心 DLL 输出目录（intelnet_core.dll）
#define RustCoreRoot "rust-core\target\release"

[Setup]
; AppId 用 GUID，卸载时识别用。去 https://www.guidgenerator.com 生成一个换掉
AppId={{fc9ca74f-5d4f-4344-979b-57f53ab78ec7}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppDirName}
DisableProgramGroupPage=yes
LicenseFile=LICENSE
OutputDir=installer\output
OutputBaseFilename=intelnet-browser-setup-{#MyAppVersion}
; SetupIconFile=resources\icon.ico
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern dynamic
; Win10 1809 (17763) 起，Qt6 最低要求
MinVersion=10.0.17763
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
; 3.5GB+ 的 models 不进安装包，首次启动由程序自己下载
; 安装包只含程序本体 + 运行库

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; 主程序 + Qt DLL + plugins + translations（windeployqt 后的完整目录全拷）
; 排除调试符号和中间文件
Source: "{#BuildRoot}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.pdb,*.exp,*.lib,*.ilk,*.obj,Makefile*,*.o,*.a,*.so"
; Rust 核心 DLL（windeployqt 不认它，单独拷）
Source: "{#RustCoreRoot}\intelnet_core.dll"; DestDir: "{app}"; Flags: ignoreversion
; vc_redist 放临时目录，装完删除，不进程序目录
Source: "installer\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
const
  // 自带的 vc_redist 版本号，更新 vc_redist.x64.exe 时同步改这里
  // 查看方法：右键 vc_redist.x64.exe → 属性 → 详细信息 → 产品版本
  // 注册表位置：HKLM\SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64 的 Version
  BundledVCRedistVersion = '14.44.35211.0';

function GetInstalledVCRedistVersion(): String;
var
  Ver: String;
begin
  Result := '';
  if RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Version', Ver) then
    Result := Ver;
end;

function CompareVersions(V1, V2: String): Integer;
var
  P1, P2, N1, N2: Integer;
  S1, S2: String;
begin
  S1 := V1; S2 := V2;
  Result := 0;
  while (S1 <> '') or (S2 <> '') do
  begin
    P1 := Pos('.', S1);
    if P1 > 0 then begin N1 := StrToIntDef(Copy(S1, 1, P1-1), 0); Delete(S1, 1, P1); end
    else begin N1 := StrToIntDef(S1, 0); S1 := ''; end;
    P2 := Pos('.', S2);
    if P2 > 0 then begin N2 := StrToIntDef(Copy(S2, 1, P2-1), 0); Delete(S2, 1, P2); end
    else begin N2 := StrToIntDef(S2, 0); S2 := ''; end;
    if N1 < N2 then begin Result := -1; Exit; end;
    if N1 > N2 then begin Result := 1; Exit; end;
  end;
end;

function NeedsVCRedist(): Boolean;
var
  Installed: String;
begin
  Installed := GetInstalledVCRedistVersion();
  // 没装过，或自带版本比已装的新，才装
  Result := (Installed = '') or (CompareVersions(BundledVCRedistVersion, Installed) > 0);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    if NeedsVCRedist() then
    begin
      WizardForm.StatusLabel.Caption := '正在安装 Visual C++ 运行库，请稍候...';
      if Exec(ExpandConstant('{tmp}\vc_redist.x64.exe'), '/quiet /norestart', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
      begin
        // 0=成功，1638=系统已有更新版本，3010=需要重启，都不算失败
        if not ((ResultCode = 0) or (ResultCode = 1638) or (ResultCode = 3010)) then
          MsgBox('Visual C++ 运行库安装返回异常码: ' + IntToStr(ResultCode) + #13#10 +
                 '程序可能无法正常启动，建议手动安装最新版 vc_redist.x64.exe 后重试。',
                 mbInformation, MB_OK);
      end
      else
        MsgBox('Visual C++ 运行库安装程序启动失败，程序可能无法正常启动。', mbInformation, MB_OK);
    end;
  end;
end;
