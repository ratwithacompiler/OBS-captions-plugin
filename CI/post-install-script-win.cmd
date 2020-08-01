REM this just creates zip files with the plugin dll (and the dependency .dlls) in the obs plugin folder structure

dir


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
