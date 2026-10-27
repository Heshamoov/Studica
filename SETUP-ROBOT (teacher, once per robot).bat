@echo off
title Set up a robot (teacher - once per robot)
cd /d "%~dp0"
color 1F
echo.
echo  =====================================================================
echo    SET UP A ROBOT  (teacher - do this ONCE per robot, or after you
echo    get a new version of the program)
echo  =====================================================================
echo.
echo   1. Switch the robot ON and wait 1 minute.
echo   2. Connect this laptop to the robot's Wi-Fi.
echo   3. Press any key here.
echo.
echo   The robot password is typed for you automatically.
echo   (Only on an old Windows: if it still asks, type  raspberry  and Enter.)
echo.
pause

where scp >nul 2>nul
if errorlevel 1 (
  echo.
  echo  ERROR: this laptop has no "scp". Windows Settings ^> Apps ^> Optional features
  echo  ^> Add a feature ^> "OpenSSH Client", then run this again.
  pause & exit /b 1
)

set ROBOT=pi@10.12.34.2
rem Answer the password prompt automatically (Windows OpenSSH 8.4 or newer).
rem Short path (no spaces), so it works from any folder on any laptop.
for %%I in ("%~dp0tools\askpass.cmd") do set "SSH_ASKPASS=%%~sI"
set SSH_ASKPASS_REQUIRE=force
set DISPLAY=localhost:0
set OPTS=-o StrictHostKeyChecking=no -o UserKnownHostsFile=NUL -o ConnectTimeout=10 -o PreferredAuthentications=password,keyboard-interactive -o NumberOfPasswordPrompts=2 -o LogLevel=ERROR
for /f "usebackq delims=" %%t in (`powershell -NoProfile -Command "Get-Date -Format 'yyyy-MM-dd HH:mm:ss'"`) do set NOW=%%t

echo.
echo  [1/2] Copying the robot program to the robot (about 1 minute)...
scp %OPTS% -r -q robot-onboard %ROBOT%:~/
if errorlevel 1 goto fail

echo.
echo  [2/2] Installing the robot helper...
ssh %OPTS% %ROBOT% "sudo bash ~/robot-onboard/manager/install.sh '%NOW%'"
if errorlevel 1 goto fail

echo.
echo  All done. The robot is now compiling its program (first time: about
echo  3-5 minutes) and will start it by itself.
echo  Open BEGINNER-GUIDE.html - it shows the progress.
echo.
start "" "BEGINNER-GUIDE.html"
pause
exit /b 0

:fail
color 4F
echo.
echo  SOMETHING WENT WRONG.
echo  - Is the laptop on the robot's Wi-Fi?
echo  - Is the robot switched on (wait 1 minute after power-up)?
echo  - Did you type the password  raspberry  correctly?
echo  Then run this file again.
pause
exit /b 1
