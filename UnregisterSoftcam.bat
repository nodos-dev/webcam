@echo off

set current_dir=%cd%
set INSTALLER=softcam_installer
set TARGET=softcam.dll

echo ################################################################
echo Softcam Installer (softcam_installer.exe) will uninstall Softcam
echo (softcam.dll) from your system.
echo ################################################################
echo.

cd /d "%~dp0Binaries"
%INSTALLER% unregister %TARGET%
cd /d "%current_dir%"

if %ERRORLEVEL% == 0 (
  echo.
  echo Successfully done.
  echo.
) else (
  echo.
  echo The process has been canceled or failed.
  echo.
)
pause
