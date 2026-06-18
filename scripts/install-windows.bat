@echo off
:: VERTIGO — Windows installer
:: Double-click this file to install VERTIGO to your VST3 folder.

echo.
echo Installing VERTIGO...
echo.

set "DEST=%APPDATA%\VST3"
if not exist "%DEST%" mkdir "%DEST%"

:: Remove old version if present
if exist "%DEST%\VERTIGO.vst3" rd /s /q "%DEST%\VERTIGO.vst3"

xcopy /E /I /Y /Q "%~dp0VERTIGO.vst3" "%DEST%\VERTIGO.vst3\"

if %ERRORLEVEL% EQU 0 (
    echo   VST3 installed to %DEST%\VERTIGO.vst3
    echo.
    echo Done! Rescan plugins in your DAW to find VERTIGO.
    echo   Ableton:  Options - Rescan Plug-ins
    echo   Bitwig:   Settings - Plug-ins - Rescan
    echo   Reaper:   Options - Preferences - Plug-ins - VST
) else (
    echo.
    echo ERROR: Installation failed. Try running this file as Administrator.
)

echo.
pause
