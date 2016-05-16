rm -rf platforms/*
rm -rf plugins/*

cordova platform add android

cd www/js
npm install
cd ../../
npm install -g coffee-script

cordova plugin add cordova-plugin-bluetoothle
cordova plugin add https://github.com/katzer/cordova-plugin-background-mode.git
cordova plugin add https://github.com/EddyVerbruggen/Insomnia-PhoneGap-Plugin.git
cordova plugin add cordova-plugin-file
cordova plugin add cordova-plugin-file-transfer
cordova plugin add cordova-plugin-vibration

cordova plugin add modified_plugins/cordova-plugin-dialogs/ --link