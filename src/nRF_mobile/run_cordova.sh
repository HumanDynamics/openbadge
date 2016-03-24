#!/bin/sh
set -e # stop on errors
cd www/js
browserify index.js -o bundle.js
cd ../..
cordova run
