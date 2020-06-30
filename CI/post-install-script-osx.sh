#!/bin/bash


set -e
set -v

#------------------
#make rpaths relative
cd build


cp libobs_google_caption_plugin.so libobs_google_caption_plugin_obs23.so
cp libobs_google_caption_plugin.so libobs_google_caption_plugin_obs24.so

#OBS 23 ------------------
otool  -L libobs_google_caption_plugin_obs23.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @rpath/QtWidgets libobs_google_caption_plugin_obs23.so
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @rpath/QtGui libobs_google_caption_plugin_obs23.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @rpath/QtCore libobs_google_caption_plugin_obs23.so
otool  -L libobs_google_caption_plugin_obs23.so

# ensure it worked
otool -L libobs_google_caption_plugin_obs23.so | grep -q '@rpath/QtWidgets'
otool -L libobs_google_caption_plugin_obs23.so | grep -q '@rpath/QtCore'
otool -L libobs_google_caption_plugin_obs23.so | grep -q '@rpath/QtGui'

#OBS 23 ------------------


#OBS 24+ ------------------
otool  -L libobs_google_caption_plugin_obs24.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets libobs_google_caption_plugin_obs24.so
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui libobs_google_caption_plugin_obs24.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore libobs_google_caption_plugin_obs24.so
otool  -L libobs_google_caption_plugin_obs24.so

# ensure it worked
otool -L libobs_google_caption_plugin_obs24.so | grep -q '@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui'
otool -L libobs_google_caption_plugin_obs24.so | grep -q '@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore'
otool -L libobs_google_caption_plugin_obs24.so | grep -q '@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets'
#OBS 24+ ------------------


mkdir release
#mkdir release/for_Mac_OBS_23
mkdir release/for_Mac_OBS_24_and_above

#cp -vn libobs_google_caption_plugin_obs23.so release/for_Mac_OBS_23/libobs_google_caption_plugin.so
cp -vn libobs_google_caption_plugin_obs24.so release/for_Mac_OBS_24_and_above/libobs_google_caption_plugin.so

cd ..
#------------------

find ./
df -lh
