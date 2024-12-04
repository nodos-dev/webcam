@echo off

reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID" /s /f "softcam.dll"

pause
