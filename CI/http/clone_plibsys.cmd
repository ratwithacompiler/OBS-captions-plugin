set PLIBSYS_PIN=%~1
set PATCH_FILE=%~dp0plibsys-socket-wakeup.patch

if exist plibsys rmdir /S /Q plibsys
git clone https://github.com/saprykin/plibsys || exit /b 1
cd plibsys
git reset --hard %PLIBSYS_PIN% || exit /b 1
git apply "%PATCH_FILE%" || exit /b 1
type nul > .patched-%PLIBSYS_PIN%
cd ..
