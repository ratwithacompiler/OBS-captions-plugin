
dir
mkdir release_dir
mkdir release_dir\32bit
mkdir release_dir\64bit

copy build_32\RelWithDebInfo\obs_google_caption_plugin.dll release_dir\32bit\obs_google_caption_plugin.dll
copy build_64\RelWithDebInfo\obs_google_caption_plugin.dll release_dir\64bit\obs_google_caption_plugin.dll

dir release_dir\32bit
dir release_dir\64bit
