/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
var badges = ['E1:C1:21:A2:B2:E0','D1:90:32:2F:F1:4B'];
//var badges = ['E1:C1:21:A2:B2:E0'];
var badgesInfo = {};
var watchdogTimer = null;

var app = {
    // Application Constructor
    initialize: function() {
        this.bindEvents();
        detailPage.hidden = true;
    },
    // Bind Event Listeners
    //
    // Bind any events that are required on startup. Common events are:
    // 'load', 'deviceready', 'offline', and 'online'.
    bindEvents: function() {
        document.addEventListener('deviceready', this.onDeviceReady, false);
        refreshButton.addEventListener('touchstart', this.refreshDeviceList, false);
        connectButton.addEventListener('touchstart', this.connect, false);
        disconnectButton1.addEventListener('touchstart', this.disconnect1, false);
        disconnectButton2.addEventListener('touchstart', this.disconnect2, false);
        watchdogStartButton.addEventListener('touchstart', this.watchdogStart, false); 
        watchdogEndButton.addEventListener('touchstart', this.watchdogEnd, false); 
        stateButton.addEventListener('touchstart', this.stateButtonPressed, false); 
        /*
        sendButton.addEventListener('click', this.sendData, false);
        isConnectedButton.addEventListener('touchstart', this.isConnected, false);
        disconnectButton.addEventListener('touchstart', this.disconnect, false);
        deviceList.addEventListener('touchstart', this.connect, false); // assume not scrolling
        */
    },
    // deviceready Event Handler
    //
    // The scope of 'this' is the event. In order to call the 'receivedEvent'
    // function, we must explicitly call 'app.receivedEvent(...);'
    onDeviceReady: function() {
        console.log("HERE!");

        // populate intervals dict
        for (var i = 0; i < badges.length; ++i) {
            console.log("Adding: "+badges[i]);
            badgesInfo[badges[i]]={};
            badgesInfo[badges[i]].lastActivity = new Date();
            badgesInfo[badges[i]].lastDisconnect = new Date();
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
                console.log('prem ok');
            },
            function(obj) {
                console.log('prem err');
            }
            );
        }
    },
    refreshDeviceList: function() {
        console.log('Start scan');
        deviceList.innerHTML = ''; // empties the list

        bluetoothle.startScan(app.startScanSuccess, app.startScanError, {});
        console.log('Start scan call end');
    },
    startScanSuccess: function(obj) {
        if (obj.status == "scanResult")
        {
            console.log("Found - "+obj.address);
            console.log("Found - "+Object.keys(obj));

            var listItem = document.createElement('li'),
                html = '<b>' + obj.name + '</b><br/>' +
                    'RSSI: ' + obj.rssi + '&nbsp;|&nbsp;' +
                    obj.address;

            //if (device.name == "BADGE") {
            if (badges.indexOf(obj.address) != -1) {
                console.log('Found: '+ obj.address);
                //app.connectToDevice(device.id);

                listItem.dataset.deviceId = obj.address;
                listItem.innerHTML = html;
                deviceList.appendChild(listItem);
            }
        }
        else if (obj.status == "scanStarted")
        {
            console.log("Scan was started successfully, stopping in 10");
            scanTimer = setTimeout(app.scanTimeout, 10000);
        }
        else
        {
            console.log("Unexpected start scan status: " + obj.status);
        }
    },
    startScanError: function(obj) {
      console.log("Start scan error: " + obj.error + " - " + obj.message);
    },
    scanTimeout: function()
    {
        function stopScanError(obj) {
            console.log("Stop scan error: " + obj.error + " - " + obj.message);
        ;}
        var stopScanSuccess = function(obj){
          if (obj.status == "scanStopped")
          {
            console.log("Scan was stopped successfully");
          }
          else
          {
            console.log("Unexpected stop scan status: " + obj.status);
          }
        };
        console.log("Scanning time out, stopping");
        bluetoothle.stopScan(stopScanSuccess, stopScanError);
    },
    connect: function() {
        app.connectDevice(badges[0]);
    },
    /*
    Connect to a Bluetooth LE device. The app should use a timer to limit the connecting time in case connecting 
    is never successful. Once a device is connected, it may disconnect without user intervention. The original 
    connection callback will be called again and receive an object with status => disconnected. To reconnect to 
    the device, use the reconnect method. If a timeout occurs, the connection attempt should be canceled using
    disconnect(). For simplicity, I recommend just using connect() and close(), don't use reconnect() or disconnect().

    */
    connectDevice: function(address)
    {
        var connectError = function(obj){
            console.log(obj.address + "|Connect error: " + obj.error + " - " + obj.message + " Keys: "+Object.keys(obj));
            app.touchLastActivity(obj.address);

            app.closeDevice(obj.address); //Best practice is to close on connection error. In our cae
                                               //we also want to reconnect afterwards
        };

        var connectSuccess = function(obj){
            console.log(obj.address + "|Connected: " + obj.status + " Keys: "+Object.keys(obj));
            app.touchLastActivity(obj.address);
            
            // Closes the device after we are done
            if (obj.status == "connected") {
                console.log(obj.address + "|Done. Call disconnect")

                app.closeDevice(obj.address);
            } else {
                console.log(obj.address + "|Unexpected disconnected")
                app.closeDevice(obj.address); //Best practice is to close on connection error. No need to
                                                    //reconnect here, it seems.
            }
        };

        console.log(address+"|Beginning connection to");
        app.touchLastActivity(address);

        var paramsObj = {"address":address};
        bluetoothle.connect(connectSuccess, connectError, paramsObj);
    },
    disconnect1: function() {
        app.closeDevice(badges[0]);
    },
    disconnect2: function() {
        app.closeDevice(badges[1]);
    },
    closeDevice: function(address)
    {
        var onCloseNoReconnectError = function(obj){
            console.log(obj.address+"|Close without reconnect error: " +  obj.error + " - " + obj.message + " Keys: "+Object.keys(obj));
            app.touchLastDisconnect(obj.address);
        };

        //    var ct = app.connectToDeviceWrap(obj.address);
        //    setTimeout(ct,2000);

        var onCloseNoReconnect = function(obj){
            console.log(obj.address+"|Close without reconnect: " + obj.status + " Keys: "+Object.keys(obj));
            app.touchLastDisconnect(obj.address);
        };

        console.log(address+"|Beginning close from");
        var paramsObj = {"address":address};
        bluetoothle.close(onCloseNoReconnect, onCloseNoReconnectError, paramsObj);
    },
    connectToDeviceWrap : function(address){
        return function() {
            app.connectDevice(address);
        }
    },
    closeDeviceWrap : function(address){
        return function() {
            app.closeDevice(address);
        }
    },    
    watchdogStart: function() {
        console.log("Starting watchdog");
        watchdogTimer = setInterval(function(){ app.watchdog() }, 1000);  
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
            var activityDatetime = badgesInfo[badge].lastActivity;
            var disconnectDatetime = badgesInfo[badge].lastDisconnect;
            console.log(badge+"|lastActivity: "+activityDatetime+"|lastDisconnect: "+disconnectDatetime);
        }
    },
    watchdog: function() {
        for (var i = 0; i < badges.length; ++i) {
            var badge = badges[i];
            var activityDatetime = badgesInfo[badge].lastActivity;
            var disconnectDatetime = badgesInfo[badge].lastDisconnect;
            console.log(badge+"|lastActivity: "+activityDatetime+"|lastDisconnect: "+disconnectDatetime);

            var nowDatetimeMs = Date.now();
            var activityDatetimeMs = activityDatetime.getTime();
            var disconnectDatetimeMs = disconnectDatetime.getTime();

            // kill if connecting for too long and/or if no activity (can update when data recieved)
            // call close() just in case
            // next watchdog call should perform connection
            if (nowDatetimeMs - activityDatetimeMs > 15000) {
                console.log(badge+"|Should timeout");

                // touch activity and disconnect date to make sure we don't keep calling this
                // and that we are not calling the next function while there's a disconnect hapenning already
                app.touchLastActivity(badge);
                app.touchLastDisconnect(badge);

                // close
                var cf = app.closeDeviceWrap(badge);
                cf();

            } else if (nowDatetimeMs - disconnectDatetimeMs > 5000 && disconnectDatetimeMs >= activityDatetimeMs) {
                    // if more than XX seconds since last disconnect 
                    // and disconnect occoured AFTER connect (meanning that there isn't an open session)
                    // call connect
                console.log(badge+"|Last disconnected XX seconds ago. Should try to connect again");

                // touch activity date so we know not to try and connect again
                app.touchLastActivity(badge);

                // connect
                var cf = app.connectToDeviceWrap(badge);
                cf();
            } else {
                console.log(badge+"|Watchdog, Do nothing");
            }
        }
        

    },
    touchLastActivity: function(address) {
        var d = new Date();
        console.log(address+"|"+"Updating last activity: "+d);
        badgesInfo[address].lastActivity = d;
    },
    touchLastDisconnect: function(address) {
        var d = new Date();
        console.log(address+"|"+"Updating last disconnect: "+d);
        badgesInfo[address].lastDisconnect = d;
    }
};

app.initialize();