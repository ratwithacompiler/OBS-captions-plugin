#!/bin/bash


set -e
set -v

#------------------
#make rpaths relative
cd build

otool  -L libobs_google_caption_plugin.so

install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @rpath/QtWidgets libobs_google_caption_plugin.so
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @rpath/QtGui libobs_google_caption_plugin.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @rpath/QtCore libobs_google_caption_plugin.so

otool  -L libobs_google_caption_plugin.so


# ensure it worked
otool -L libobs_google_caption_plugin.so | grep -q '@rpath/QtWidgets'
otool -L libobs_google_caption_plugin.so | grep -q '@rpath/QtCore'
otool -L libobs_google_caption_plugin.so | grep -q '@rpath/QtGui'

cd ..
#------------------

find ./