
set DepsBaseOBS=%cd%\obs_deps
echo DepsBaseOBS %DepsBaseOBS%

set DepsBaseGRPCx64=%cd%\grpc_deps_x64
set DepsBaseGRPCx86=%cd%\grpc_deps_x86
echo DepsBaseGRPCx64 %DepsBaseGRPCx64%
echo DepsBaseGRPCx86 %DepsBaseGRPCx86%

dir
dir %DepsBaseGRPCx64%
dir %DepsBaseGRPCx86%

if defined GOOGLE_API_KEY (
  if "%GOOGLE_API_KEY%" == "$(GOOGLE_API_KEY)" (
    echo ignoring azure env arg
  ) else (
    echo building with hardcoded compiled API key
    set API_OR_UI_KEY_ARG="-DGOOGLE_API_KEY=%GOOGLE_API_KEY%"
  )
)

if not defined API_OR_UI_KEY_ARG (
    echo building with custom user API key UI
    set API_OR_UI_KEY_ARG=-DENABLE_CUSTOM_API_KEY=ON
)

:: 64 bit build
mkdir build_64
cd build_64

cmake.exe ../../../ ^
-G "Visual Studio 16 2019" -A x64 ^
-DSPEECH_API_GOOGLE_GRPC_V1=ON ^
-DOBS_SOURCE_DIR='%DepsBaseOBS%\obs_src\' ^
-DOBS_LIB_DIR='%DepsBaseOBS%\obs_src\build_64\' ^
-DQT_DEP_DIR='%DepsBaseOBS%\Qt\5.15.2\msvc2019_64' ^
-DGOOGLEAPIS_DIR='%DepsBaseGRPCx64%\googleapis' ^
-DGRPC_CMAKE_INCLUDE='%DepsBaseGRPCx64%\vcpkg_export\scripts\buildsystems\vcpkg.cmake' ^
-DVCPKG_TARGET_TRIPLET=x64-windows-static-md ^
-DBUILD_SHARED_LIBS=ON ^
%API_OR_UI_KEY_ARG%

if %errorlevel% neq 0 exit /b %errorlevel%

REM -DQT_DEP_DIR='%DepsBaseOBS%\Qt\5.15.2\msvc2019_64' ^
REM -DVCPKG_TARGET_TRIPLET=x64-windows-static ^

if %errorlevel% neq 0 (
    echo cmake 64 failed
    cd ..
    exit /b %errorlevel%
)
cd ..
cd

:: 32 bit build
mkdir build_32
cd build_32

cmake.exe ../../../ ^
-G "Visual Studio 16 2019" -A Win32 ^
-DSPEECH_API_GOOGLE_GRPC_V1=ON ^
-DOBS_SOURCE_DIR='%DepsBaseOBS%\obs_src\' ^
-DOBS_LIB_DIR='%DepsBaseOBS%\obs_src\build_32\' ^
-DQT_DEP_DIR='%DepsBaseOBS%\Qt\5.15.2\msvc2019' ^
-DGOOGLEAPIS_DIR='%DepsBaseGRPCx86%\googleapis' ^
-DGRPC_CMAKE_INCLUDE='%DepsBaseGRPCx86%\vcpkg_export\scripts\buildsystems\vcpkg.cmake' ^
-DVCPKG_TARGET_TRIPLET=x86-windows-static-md ^
-DBUILD_SHARED_LIBS=ON ^
%API_OR_UI_KEY_ARG%

if %errorlevel% neq 0 exit /b %errorlevel%

REM -DQT_DEP_DIR='%DepsBaseOBS%\Qt\5.15.2\msvc2019' ^
REM -DVCPKG_TARGET_TRIPLET=x86-windows-static ^

if %errorlevel% neq 0 (
    echo cmake 32 failed
    cd ..
    exit /b %errorlevel%
)
cd ..
cd

:: copy the cmake processed file with version_string
copy build_64\CI\grpc\win-install-script.cmd win-install-script.cmd

cd build_64
cmake --build . --config RelWithDebInfo
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

cd build_32
cmake --build . --config RelWithDebInfo
if %errorlevel% neq 0 exit /b %errorlevel%
cd ..

dir
REM this just creates zip files with the plugin dll (and the dependency .dlls) in the obs plugin folder structure

set OBSPLUGIN=release\Closed_Captions_Plugin__v${VERSION_STRING}_Windows\obs-plugins\
echo plugin path: %OBSPLUGIN%\

mkdir %OBSPLUGIN%\
mkdir %OBSPLUGIN%\32bit
mkdir %OBSPLUGIN%\64bit



copy build_32\RelWithDebInfo\obs_google_caption_plugin.dll %OBSPLUGIN%\32bit\obs_google_caption_plugin.dll
copy build_64\RelWithDebInfo\obs_google_caption_plugin.dll %OBSPLUGIN%\64bit\obs_google_caption_plugin.dll

REM dir %OBSPLUGIN%\32bit
REM dir %OBSPLUGIN%\64bit

7z a -r release\Closed_Captions_Plugin__v${VERSION_STRING}_Windows.zip %CD%\release\Closed_Captions_Plugin__v${VERSION_STRING}_Windows
7z a -r release\Closed_Captions_Plugin__v${VERSION_STRING}_Windows_plugins.zip %CD%\release\Closed_Captions_Plugin__v${VERSION_STRING}_Windows\obs-plugins


dir d:\
