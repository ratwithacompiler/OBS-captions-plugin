
set DepsBaseOBS=%cd%\obs_deps\
echo DepsBaseOBS %DepsBaseOBS%

set GrpcBase=%cd%\grpc_deps
echo GrpcBase %GrpcBase%

dir

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
    set API_OR_UI_KEY_ARG="-DENABLE_CUSTOM_API_KEY=ON"
)


:: 64 bit build
mkdir build_64
cd build_64


cmake.exe ../../  -G "Visual Studio 15 2017 Win64" ^
-DSPEECH_API_GOOGLE_GRPC_V1=ON ^
-DOBS_SOURCE_DIR='%DepsBaseOBS%\obs_src\' ^
-DOBS_LIB_DIR='%DepsBaseOBS%\obs_src\build_64\' ^
-DQT_DIR='%DepsBaseOBS%\Qt\5.10.1\msvc2017_64' ^
-DGRPC_ROOT_DIR='%GrpcBase%\x64-windows-static' ^
-DPROTOBUF_ROOT_DIR='%GrpcBase%\x64-windows-static' ^
-DGOOGLEAPIS_DIR='%GrpcBase%\googleapis' ^
%API_OR_UI_KEY_ARG%
REM -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo cmake 64 failed
    cd ..
    exit /b %errorlevel%
)

cd ..
cd
REM dir build_64
REM cd




:: 32 bit build
mkdir build_32
cd build_32
cd

cmake.exe ../../  -G "Visual Studio 15 2017" ^
-DSPEECH_API_GOOGLE_GRPC_V1=ON ^
-DOBS_SOURCE_DIR='%DepsBaseOBS%\obs_src\' ^
-DOBS_LIB_DIR='%DepsBaseOBS%\obs_src\build_32\' ^
-DQT_DIR='%DepsBaseOBS%\Qt\5.10.1\msvc2017' ^
-DGRPC_ROOT_DIR='%GrpcBase%\x86-windows-static' ^
-DPROTOBUF_ROOT_DIR='%GrpcBase%\x86-windows-static' ^
-DGOOGLEAPIS_DIR='%GrpcBase%\googleapis' ^
%API_OR_UI_KEY_ARG%
REM -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo cmake 32 failed
    cd ..
    exit /b %errorlevel%
)

cd ..
cd
REM dir build_32
REM cd

:: copy the cmake processed file with version_string
copy build_64\CI\grpc\post-install-script-win.cmd grpc\post-install-script-win.cmd
REM type post-install-script-win.cmd

dir

