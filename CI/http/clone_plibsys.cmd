@echo off
rem Windows twin of clone_plibsys.sh (same pin), plus — Windows only — the
rem repo's plibsys-socket-wakeup.patch (interruptible socket waits) and a
rem sentinel file. Called by win_shared.ensure_plibsys with the pin as %1
rem and CWD = the CI_build dir.
setlocal
if "%~1"=="" (
  echo usage: clone_plibsys.cmd ^<plibsys commit pin^>
  exit /b 1
)
set PLIBSYS_PIN=%~1
set PATCH_FILE=%~dp0plibsys-socket-wakeup.patch

if exist plibsys rmdir /S /Q plibsys
git clone https://github.com/saprykin/plibsys || exit /b 1
cd plibsys
git reset --hard %PLIBSYS_PIN% || exit /b 1
git apply "%PATCH_FILE%" || exit /b 1
type nul > .patched-%PLIBSYS_PIN%
cd ..
