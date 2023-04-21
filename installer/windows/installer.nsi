; SPDX-License-Identifier: Apache-2.0

!include Sections.nsh

!ifndef PROGRAM_NAME
  !define PROGRAM_NAME wahjam2
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

OutFile ${PROGRAM_NAME}-${VERSION}-installer.exe
Name ${PROGRAM_NAME}
InstallDir "$LOCALAPPDATA\${PROGRAM_NAME}"
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
  DeleteRegKey HKCU "Software\${PROGRAM_NAME}\${PROGRAM_NAME}"
; TODO Should be "Software\Wahjam Project\Wahjam2"
  DeleteRegKey /ifempty HKCU "Software\${PROGRAM_NAME}"
  Delete "$SMPROGRAMS\${PROGRAM_NAME}.lnk"

  SetOutPath $INSTDIR
  Delete license.txt
  Delete "*.exe"
  Delete "*.dll"
  RMDir /r "$INSTDIR\cache"
  RMDir /r "$INSTDIR\plugins"
  RMDir /r "$INSTDIR\qml"
  Delete uninstall.exe
  Delete "${PROGRAM_NAME}\log.txt"
  RMDir "$INSTDIR\${PROGRAM_NAME}"
  SetOutPath $TEMP
  RMDir $INSTDIR
SectionEnd

Function .onInit
  ; Try to get focus in case we've been hidden behind another window
  BringToFront
FunctionEnd
