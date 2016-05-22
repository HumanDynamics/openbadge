#!/bin/sh
set -e # stop on errors
cd www/js
coffee --compile mm.coffee
browserify index.js -o bundle.js
cd ../..
cordova run
rm www/js/bundle.js
rm www/js/mm.js