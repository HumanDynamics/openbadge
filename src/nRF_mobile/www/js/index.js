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
//var badges = ['E1:C1:21:A2:B2:E0','D1:90:32:2F:F1:4B'];
var badges = ['E1:C1:21:A2:B2:E0'];
var badgesTimeouts = {};
var connectTimer = null;

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
        disconnectButton.addEventListener('touchstart', this.disconnect, false);
        autogoButton.addEventListener('touchstart', this.autogo, false); 
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
            badgesTimeouts[badges[i]]=null;
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

            //console.log("Stopping scan..");
            //bluetoothle.stopScan(stopScanSuccess, stopScanError);
            //clearScanTimeout();

            //window.localStorage.setItem(addressKey, obj.address);
            //    connectDevice(obj.address);

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
            console.log("Connect error: " + obj.error + " - " + obj.message + " Keys: "+Object.keys(obj));
            app.clearConnectTimeout();

            app.closeDevice(obj.address,true); //Best practice is to close on connection error. In our cae
                                               //we also want to reconnect afterwards
        };

        var connectSuccess = function(obj){
            console.log("Connected: " + obj.status + " - " + obj.address + " Keys: "+Object.keys(obj));
            app.clearConnectTimeout();
            
            // Closes the device after we are done
            if (obj.status == "connected") {
                console.log("Done. Call disconnect")

                app.closeDevice(obj.address, true);
            } else {
                console.log("Unexpected disconnected")
                app.closeDevice(obj.address,false); //Best practice is to close on connection error. No need to
                                                    //reconnect here, it seems.
            }
        };

        console.log("Beginning connection to: " + address);
        var paramsObj = {"address":address};
        bluetoothle.connect(connectSuccess, connectError, paramsObj);

        // timer for killing connections that take too long
        console.log("Setting timeout (timer was: "+connectTimer+")");
        //console.log("Setting timeout");
        connectTimer = setTimeout(app.connectTimeout, 5000);
    },
    connectTimeout: function()
    {
      console.log("Connection timed out");
      app.closeDevice(badges[0],true); //TODO - change to a parameter!
    },
    clearConnectTimeout: function()
    { 
        console.log("Clearing connect timeout");
        if (connectTimer != null)
        {
            clearTimeout(connectTimer);
        }
    },



    disconnect: function() {
        app.closeDevice(badges[0],false);
    },
    closeDevice: function(address,reconnect)
    {
        var onCloseReconnectError = function(obj){
            console.log("Close with reconnect error: " + obj.error + " - " + obj.message + " Keys: "+Object.keys(obj));
            // setting a timeout with several seconds before reconecting
            setTimeout(app.connectToDeviceWrap(obj.address),2000);
        };

        var onCloseNoReconnectError = function(obj){
            console.log("Close without reconnect error: " + obj.error + " - " + obj.message + " Keys: "+Object.keys(obj));
        };

        var onCloseReconnect = function(obj){
            console.log("Close with reconnect: " + obj.status + " - " + obj.address + " Keys: "+Object.keys(obj));
            // setting a timeout with several seconds before reconecting
            setTimeout(app.connectToDeviceWrap(obj.address),2000);
        };

        var onCloseNoReconnect = function(obj){
            console.log("Close without reconnect: " + obj.status + " - " + obj.address + " Keys: "+Object.keys(obj));
        };

        console.log("Beginning close from: " + address + " Reconnect: "+reconnect);
        var paramsObj = {"address":address};
        if (reconnect) {
            bluetoothle.close(onCloseReconnect, onCloseReconnectError, paramsObj);
        } else {
            bluetoothle.close(onCloseNoReconnect, onCloseNoReconnectError, paramsObj);
        }
        
    },
    connectToDeviceWrap : function(deviceId){
        return function() {
            app.connectDevice(deviceId);
        }
    },
    autogo: function() {
        for (var i = 0; i < badges.length; ++i) {
            var badge=badges[i];
            var f = app.connectToDeviceWrap(badge);
            console.log("initializing connection to "+badge);
            //var interval = setInterval(f,1000);
            f();

            //var f2 = app.disconnectFromDeviceWrap(badge);
            //console.log("Starting timer for disconnecting from "+badge);
            //var interval2 = setInterval(f2,1500);            
        }
    }
};

app.initialize();