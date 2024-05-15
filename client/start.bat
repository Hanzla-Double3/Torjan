@echo off

set appPath="C:\Program Files\WindowsClient\client.exe"
set taskName="Client Startup Task"

REM Check if task already exists
schtasks /query /TN %taskName% >nul 2>&1
if %errorlevel% equ 0 (
    echo Task already exists
    goto :EOF
)

REM Create task
schtasks /create /tn %taskName% /tr %appPath% /sc onstart /ru "SYSTEM"

echo Task created successfully
pause
