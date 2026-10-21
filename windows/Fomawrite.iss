; Internal per-user installer. Compile with bin/windows/build-installer.ps1.
; Installation never removes user settings, recovery or writing files.
#ifndef StageDir
  #error StageDir is required
#endif
#ifndef OutputDir
  #error OutputDir is required
#endif
#ifndef AppVersion
  #error AppVersion is required
#endif
#ifndef AppNumericVersion
  #error AppNumericVersion is required
#endif
#ifndef BuildCommit
  #error BuildCommit is required
#endif
#ifndef Architecture
  #error Architecture is required
#endif

[Setup]
AppId=io.github.andrewjngray.fomawrite.windows.{#Architecture}
AppName=Fomawrite ({#Architecture})
AppVersion={#AppVersion}
AppVerName=Fomawrite {#AppVersion} ({#Architecture})
AppPublisher=Andrew Gray
AppPublisherURL=https://github.com/andrewjngray/fomawrite_mac
VersionInfoVersion={#AppNumericVersion}
VersionInfoDescription=Fomawrite internal Windows installer
VersionInfoProductName=Fomawrite
VersionInfoProductVersion={#AppVersion}
DefaultDirName={localappdata}\Programs\Fomawrite-{#Architecture}
DefaultGroupName=Fomawrite ({#Architecture})
PrivilegesRequired=lowest
DisableProgramGroupPage=yes
AllowNoIcons=yes
CloseApplications=yes
RestartApplications=no
UninstallDisplayIcon={app}\Fomawrite.exe
UninstallDisplayName=Fomawrite ({#Architecture})
OutputDir={#OutputDir}
OutputBaseFilename=Fomawrite-{#AppVersion}-windows-{#Architecture}-{#BuildCommit}-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
MinVersion=10.0
#if Architecture == "arm64"
ArchitecturesAllowed=arm64
ArchitecturesInstallIn64BitMode=arm64
#elif Architecture == "x64"
; Includes x64-on-ARM emulation; the artifact remains explicitly labelled x64.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#else
  #error Unsupported Architecture
#endif

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Fomawrite"; Filename: "{app}\Fomawrite.exe"
Name: "{group}\Uninstall Fomawrite"; Filename: "{uninstallexe}"

; No automatic launch, document associations, settings cleanup, or user-data
; deletion. Normal launch and upgrade/uninstall are separate acceptance gates.
