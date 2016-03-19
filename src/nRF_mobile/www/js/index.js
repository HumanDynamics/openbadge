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
        /*
        sendButton.addEventListener('click', this.sendData, false);
        connectButton.addEventListener('touchstart', this.connect, false);
        isConnectedButton.addEventListener('touchstart', this.isConnected, false);
        disconnectButton.addEventListener('touchstart', this.disconnect, false);
        deviceList.addEventListener('touchstart', this.connect, false); // assume not scrolling
        autogoButton.addEventListener('touchstart', this.autogo, false); 
        */
    },
    // deviceready Event Handler
    //
    // The scope of 'this' is the event. In order to call the 'receivedEvent'
    // function, we must explicitly call 'app.receivedEvent(...);'
    onDeviceReady: function() {
        console.log("HERE!");

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
    /*
    onDiscoverDevice: function(device) {
        var listItem = document.createElement('li'),
            html = '<b>' + device.name + '</b><br/>' +
                'RSSI: ' + device.rssi + '&nbsp;|&nbsp;' +
                device.id;

        //if (device.name == "BADGE") {
        if (badges.indexOf(device.id) != -1) {
            app.showStatusText('Found: '+ device.id);
            //app.connectToDevice(device.id);

            listItem.dataset.deviceId = device.id;
            listItem.innerHTML = html;
            deviceList.appendChild(listItem);
        }
    }
    */
};

app.initialize();