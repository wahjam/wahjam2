; SPDX-License-Identifier: Apache-2.0

!include Sections.nsh

!ifndef PROGRAM_NAME
  !define PROGRAM_NAME wahjam2
!endif
!ifndef ORG_NAME
  !define ORG_NAME "Wahjam Project"
!endif
!ifndef EXECUTABLE
  !define EXECUTABLE "${PROGRAM_NAME}.exe"
!endif
!ifdef VERSION
  VIProductVersion "${VERSION}"
  Caption "${PROGRAM_NAME} ${VERSION} Setup"
!endif
!ifndef BUILD_DIR
  !define BUILD_DIR "..\..\build"
!endif

!define REG_UNINSTALL "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PROGRAM_NAME}"

; Install for current user without User Account Control elevation
RequestExecutionLevel user

OutFile "${BUILD_DIR}\${PROGRAM_NAME}-${VERSION}-installer.exe"
Name ${PROGRAM_NAME}
InstallDir "$LOCALAPPDATA\${ORG_NAME}\${PROGRAM_NAME}"
SetCompressor /SOLID lzma
LicenseData license.txt

Page license
Page components
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "-Install"
  SetOutPath $INSTDIR
  File "${BUILD_DIR}\install\*.dll"
  File "${BUILD_DIR}\install\*.exe"
  File /r "${BUILD_DIR}\install\qml"
  File /r "${BUILD_DIR}\install\plugins"
  File license.txt
  CreateShortCut "$SMPROGRAMS\${PROGRAM_NAME}.lnk" "$INSTDIR\${EXECUTABLE}"
  WriteUninstaller uninstall.exe

  ; Add/Remove Programs
  WriteRegStr HKCU "${REG_UNINSTALL}" "DisplayName" "${PROGRAM_NAME}"
  WriteRegStr HKCU "${REG_UNINSTALL}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
  WriteRegStr HKCU "${REG_UNINSTALL}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
  WriteRegStr HKCU "${REG_UNINSTALL}" "InstallLocation" "$\"$INSTDIR$\""
  WriteRegDWORD HKCU "${REG_UNINSTALL}" "NoModify" 1
  WriteRegDWORD HKCU "${REG_UNINSTALL}" "NoRepair" 1
SectionEnd

Section "Desktop shortcut"
  CreateShortCut "$DESKTOP\${PROGRAM_NAME}.lnk" "$INSTDIR\${EXECUTABLE}"
SectionEnd

Section "Launch ${PROGRAM_NAME} on completion"
  Exec '"$INSTDIR\${EXECUTABLE}"'
SectionEnd

Section Uninstall
  Delete "$DESKTOP\${PROGRAM_NAME}.lnk"

  DeleteRegKey HKCU "${REG_UNINSTALL}"
  DeleteRegKey HKCU "Software\${ORG_NAME}\${PROGRAM_NAME}"
  DeleteRegKey /ifempty HKCU "Software\${ORG_NAME}"
  Delete "$SMPROGRAMS\${PROGRAM_NAME}.lnk"

  SetOutPath $INSTDIR
  Delete license.txt
  Delete log.txt
  Delete "*.exe"
  Delete "*.dll"
  RMDir /r "$INSTDIR\cache"
  RMDir /r "$INSTDIR\plugins"
  RMDir /r "$INSTDIR\qml"
  Delete uninstall.exe
  SetOutPath $TEMP
  RMDir $INSTDIR
  RMDir "$INSTDIR\.."
SectionEnd

Function .onInit
  ; Try to get focus in case we've been hidden behind another window
  BringToFront
FunctionEnd
