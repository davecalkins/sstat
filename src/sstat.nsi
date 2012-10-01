Name                 "sstat"
OutFile              "sstat-installer.exe"
InstallDir           $PROGRAMFILES\sstat
DirText              "This will install sstat on your computer.  Choose a directory."
CRCCheck             force
SetCompress          force
SetCompressor        lzma
LicenseData          installer_text.rtf
LicenseText          Overview Continue

Section ""
   SetOutPath        $INSTDIR
   File              "Release\sstat.exe"
   File              "down.wav"
   File              "up.wav"
   File              "sstat*.cfg"
   File              "LICENSE.TXT"
   File              "HISTORY.TXT"

   CreateShortcut    "$STARTMENU\Programs\sstat.lnk" "$INSTDIR\sstat.exe" sstat.cfg
   CreateShortcut    "$DESKTOP\sstat.lnk" "$INSTDIR\sstat.exe" sstat.cfg

   WriteUninstaller  Uninst.exe
   WriteRegStr       HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\sstat" "DisplayName" "sstat"
   WriteRegStr       HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\sstat" "UninstallString" "$INSTDIR\Uninst.exe"
SectionEnd

Section "Uninstall"
   RMDir             /r $INSTDIR
   Delete            $STARTMENU\Programs\sstat.lnk
   Delete            $DESKTOP\sstat.lnk
   DeleteRegKey      HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\sstat"
SectionEnd

Function un.onInit
   MessageBox MB_YESNO "This will uninstall sstat. Continue?" IDYES NoAbort
      Abort ; causes uninstaller to quit.
   NoAbort:
FunctionEnd
