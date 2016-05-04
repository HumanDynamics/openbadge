var Q = require('q');
var qbluetoothle = require('./qbluetoothle');
var Badge = require('./badge');

//var badges = ['E1:C1:21:A2:B2:E0','D1:90:32:2F:F1:4B'];
//var badgeAddresses = ['EC:21:82:A8:0B:59','D1:90:32:2F:F1:4B'];
//var badgeAddresses = ['D1:90:32:2F:F1:4B'];
//var badgeAddresses = ['F2:42:EC:DA:04:35'];
var badgeAddresses = ['C8:62:29:B6:AC:B4'];
var badges = [];
//var badges = ['E1:C1:21:A2:B2:E0'];

var watchdogTimer = null;

var WATCHDOG_INACTIVITY_TIMEOUT = 3000; // kill after 2 ms of no activity
var WATCHDOG_RECONNECT_WAIT = 2000; // Reconnect after X ms
var WATCHDOG_SLEEP = 500; // Check status every X ms

var app = {
    // Application Constructor
    initialize: function() {
        this.bindEvents();
        //detailPage.hidden = true;
    },
    // Bind Event Listeners
    //
    // Bind any events that are required on startup. Common events are:
    // 'load', 'deviceready', 'offline', and 'online'.
    bindEvents: function() {
        document.addEventListener('deviceready', this.onDeviceReady, false);
        refreshButton.addEventListener('touchstart', this.refreshDeviceList, false);

        connectButton1.addEventListener('touchstart', this.connectButtonPressed1, false);
        connectButton2.addEventListener('touchstart', this.connectButtonPressed2, false);
        
        disconnectButton1.addEventListener('touchstart', this.disconnect1ButtonPressed, false);
        disconnectButton2.addEventListener('touchstart', this.disconnect2ButtonPressed, false);

        watchdogStartButton.addEventListener('touchstart', this.watchdogStart, false); 
        watchdogEndButton.addEventListener('touchstart', this.watchdogEnd, false); 
    },
    // deviceready Event Handler
    //
    // The scope of 'this' is the event. In order to call the 'receivedEvent'
    // function, we must explicitly call 'app.receivedEvent(...);'
    onDeviceReady: function() {
        // populate intervals dict
        for (var i = 0; i < badgeAddresses.length; ++i) {
            var address = badgeAddresses[i];
            console.log("Adding: "+address);
            badges.push(new Badge.Badge(address));
        }

        bluetoothle.initialize(
            app.initializeSuccess, 
            {request: true,statusReceiver: true}
        );
    },
    initializeSuccess: function (obj) {
        console.log('Success');

        // Android v6.0 required requestPermissions. If it's Android < 5.0 there'll
        // be an error, but don't worry about it.
        if (cordova.platformId === 'android') {
            console.log('Asking for permissions');            
            bluetoothle.requestPermission(
            function(obj) {
                console.log('permissions ok');
            },
            function(obj) {
                console.log('permissions err');
            }
            );
        }
    },
    refreshDeviceList: function() {
        deviceList.innerHTML = ''; // empties the list
        qbluetoothle.startScan().then(
            function(obj){ // success
                console.log("Scan completed successfully - "+obj.status)
            }, function(obj) { // error
                console.log("Scan Start error: " + obj.error + " - " + obj.message)
            }, function(obj) { // progress
                if (badges.indexOf(obj.address) != -1) {
                        var listItem = document.createElement('li'),
                            html = '<b>' + obj.name + '</b><br/>' +
                            'RSSI: ' + obj.rssi + '&nbsp;|&nbsp;' +
                            obj.address;

                        console.log('Found: '+ obj.address);

                        listItem.dataset.deviceId = obj.address;
                        listItem.innerHTML = html;
                        deviceList.appendChild(listItem);
                }
        });
    },

    connectButtonPressed1: function() {
        var badge = badges[0];
        console.log("will try to connect - "+badge.address);
        badge.connectDialog();
    },
    connectButtonPressed2: function() {
        var badge = badges[1];
        console.log("will try to connect - "+badge.address);
        badge.connectDialog();
    },
    disconnect1ButtonPressed: function() {
        var badge = badges[0];
        badge.close();
    },
    disconnect2ButtonPressed: function() {
        var badge = badges[1];
        badge.close();
    },
    connectToDeviceWrap : function(badge){
        return function() {
            badge.connectDialog();
        }
    },
    closeDeviceWrap : function(badge){
        return function() {
            badge.close();
        }
    },    
    watchdogStart: function() {
        console.log("Starting watchdog");
        watchdogTimer = setInterval(function(){ app.watchdog() }, WATCHDOG_SLEEP);  
    },
    watchdogEnd: function() {
        console.log("Ending watchdog");
        if (watchdogTimer != null)
        {
            clearInterval(watchdogTimer);
        }
    },
    stateButtonPressed: function() {
        for (var i = 0; i < badges.length; ++i) {
            var badge = badges[i];
            var activityDatetime = badge.lastActivity;
            var disconnectDatetime = badge.lastDisconnect;
            console.log(badge.address+"|lastActivity: "+activityDatetime+"|lastDisconnect: "+disconnectDatetime);
        }
    },
    watchdog: function() {
        // Stop watchdog timer
        clearInterval(watchdogTimer);

        // Iterate over badges
        for (var i = 0; i < badges.length; ++i) {
            var badge = badges[i];
            var activityDatetime = badge.lastActivity;
            var disconnectDatetime = badge.lastDisconnect;
            //console.log(badge.address+"|lastActivity: "+activityDatetime+"|lastDisconnect: "+disconnectDatetime);

            var nowDatetimeMs = Date.now();
            var activityDatetimeMs = activityDatetime.getTime();
            var disconnectDatetimeMs = disconnectDatetime.getTime();

            // kill if connecting for too long and/or if no activity (can update when data recieved)
            // call close() just in case
            // next watchdog call should perform connection
            if (nowDatetimeMs - activityDatetimeMs > WATCHDOG_INACTIVITY_TIMEOUT) {
                console.log(badge.address+"|Should timeout");

                // touch activity and disconnect date to make sure we don't keep calling this
                // and that we are not calling the next function while there's a disconnect hapenning already
                badge.touchLastActivity();
                badge.touchLastDisconnect();

                // close
                var cf = app.closeDeviceWrap(badge);
                cf();

            } else if (nowDatetimeMs - disconnectDatetimeMs > WATCHDOG_RECONNECT_WAIT && disconnectDatetimeMs >= activityDatetimeMs) {
                    // if more than XX seconds since last disconnect 
                    // and disconnect occoured AFTER connect (meanning that there isn't an open session)
                    // call connect
                console.log(badge.address+"|Last disconnected XX seconds ago. Should try to connect again");

                // touch activity date so we know not to try and connect again
                //badge.touchLastActivity();

                // connect
                var cf = app.connectToDeviceWrap(badge);
                cf();
            } else {
                //console.log(badge.address+"|Watchdog, Do nothing");
            }
        }
        app.watchdogStart(); // Re-set watchdog timer
    },
};


app.initialize();