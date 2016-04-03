// Based on BluefruitLE by Don Coleman (https://github.com/don/cordova-plugin-ble-central/tree/master/examples/bluefruitle)

/* global mainPage, deviceList, refreshButton, statusText */
/* global detailPage, resultDiv, messageInput, sendButton, disconnectButton */
/* global ble  */
/* jshint browser: true , devel: true*/
'use strict';

// ASCII only
function bytesToString(buffer) {
    return String.fromCharCode.apply(null, new Uint8Array(buffer));
}

// ASCII only
function stringToBytes(string) {
    var array = new Uint8Array(string.length);
    for (var i = 0, l = string.length; i < l; i++) {
        array[i] = string.charCodeAt(i);
    }
    return array.buffer;
}

// this is Nordic's UART service
var bluefruit = {
    serviceUUID: '6e400001-b5a3-f393-e0a9-e50e24dcca9e',
    txCharacteristic: '6e400002-b5a3-f393-e0a9-e50e24dcca9e', // transmit is from the phone's perspective
    rxCharacteristic: '6e400003-b5a3-f393-e0a9-e50e24dcca9e'  // receive is from the phone's perspective
};

//var badges = ['D1:90:32:2F:F1:4B','E1:C1:21:A2:B2:E0'];
var badges = ['E1:C1:21:A2:B2:E0','D1:90:32:2F:F1:4B'];
var badgesConnStat = {};

var ConnStateEnum = {
    DISCONNECTED :1,
    DISCONNECTING : 2,
    CONNECTING : 3,
    CONNECTED : 4,
};


var app = {
    initialize: function() {
        this.bindEvents();
        detailPage.hidden = true;
    },
    bindEvents: function() {
        document.addEventListener('deviceready', this.onDeviceReady, false);
        refreshButton.addEventListener('touchstart', this.refreshDeviceList, false);
        sendButton.addEventListener('click', this.sendData, false);
        connectButton.addEventListener('touchstart', this.connect, false);
        isConnectedButton.addEventListener('touchstart', this.isConnected, false);
        disconnectButton.addEventListener('touchstart', this.disconnect, false);
        deviceList.addEventListener('touchstart', this.connect, false); // assume not scrolling
        autogoButton.addEventListener('touchstart', this.autogo, false); 
    },
    onDeviceReady: function() {
        app.showStatusText("Ready");

        // populate status dict
        for (var i = 0; i < badges.length; ++i) {
            console.log("Adding: "+badges[i]);
            badgesConnStat[badges[i]]=ConnStateEnum.DISCONNECTED;
        }

        console.log(badgesConnStat);
    },

    refreshDeviceList: function() {
        app.showStatusText("Starting scan");
        deviceList.innerHTML = ''; // empties the list
        /*
        if (cordova.platformId === 'android') { // Android filtering is broken
            ble.scan([], 5, app.onDiscoverDevice, app.onError);
        } else {
            ble.scan([bluefruit.serviceUUID], 5, app.onDiscoverDevice, app.onError);
        }*/
        ble.startScan([], app.onDiscoverDevice, app.onError);
        console.log('Start scan call end');
    },

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
    },

    onConnect : function(d) { // creating a closure
                return function(peripheral) {
                    console.log('Connected '+ d + "status is still " + badgesConnStat[d]+" also - "+peripheral.id);
                    badgesConnStat[d] = ConnStateEnum.CONNECTED;
                    //app.determineWriteType(peripheral);

                    // subscribe for incoming data
                    //ble.startNotification(d, bluefruit.serviceUUID, bluefruit.rxCharacteristic, app.onData, app.onError);
                    //sendButton.dataset.d = d;
                    //disconnectButton.dataset.d = d;
                    //resultDiv.innerHTML = "";
                    //app.showDetailPage();

                    //app.disconnectFromDevice(d);
                }
            },
    onConnectError: function(d) { // creating a closure
                return function(reason) {
                    console.log('Error Connecting '+ d + " "+reason);
                    badgesConnStat[d] = ConnStateEnum.DISCONNECTED;
                }
    },

    connectToDevice: function(deviceId) {
        console.log('Attempting to connect '+ deviceId + " " + badgesConnStat[deviceId]);
        if (badgesConnStat[deviceId] == ConnStateEnum.DISCONNECTED) {
            console.log('Connecting '+ deviceId);
            badgesConnStat[deviceId] = ConnStateEnum.CONNECTING;
            var onConnect = app.onConnect(deviceId);
            var onConnectError = app.onConnectError(deviceId);
            ble.connect(deviceId, onConnect, onConnectError);
        } else {
            console.log('Will not try to connect. Already existing active connection '+ deviceId + ", Status: "+badgesConnStat[deviceId]);
        }
    },

    disconnectFromDevice: function(deviceId) {
        console.log('Attemping to disconnect '+ deviceId);
        var onDisconnect = function(d) {
            return function(e) {
                console.log("Disconnected from "+ d +" "+ e);
                badgesConnStat[d] = ConnStateEnum.DISCONNECTED;
            }
        };

        var onDisconnectError = function(d) {
            return function(reason) {
                app.showStatusText('Error disconnecting '+ d + " "+reason);
            }
        };

        if (badgesConnStat[deviceId] == ConnStateEnum.CONNECTED) {
            console.log('Disconnecting '+ deviceId);
            badgesConnStat[deviceId] = ConnStateEnum.DISCONNECTING;
            ble.disconnect(deviceId, onDisconnect(deviceId), onDisconnectError(deviceId));
        } else {
            console.log('Will not try to disconnect '+ deviceId + ", Status: "+badgesConnStat[deviceId]);
        }
    },

    determineWriteType: function(peripheral) {
        // Adafruit nRF8001 breakout uses WriteWithoutResponse for the TX characteristic
        // Newer Bluefruit devices use Write Request for the TX characteristic

        var characteristic = peripheral.characteristics.filter(function(element) {
            if (element.characteristic.toLowerCase() === bluefruit.txCharacteristic) {
                return element;
            }
        })[0];

        if (characteristic.properties.indexOf('WriteWithoutResponse') > -1) {
            app.writeWithoutResponse = true;
        } else {
            app.writeWithoutResponse = false;
        }

    },
    onData: function(data) { // data received from Arduino
        console.log(data);
        resultDiv.innerHTML = resultDiv.innerHTML + "Received: " + bytesToString(data) + "<br/>";
        resultDiv.scrollTop = resultDiv.scrollHeight;
    },
    sendData: function(event) { // send data to Arduino

        var success = function() {
            console.log("success");
            resultDiv.innerHTML = resultDiv.innerHTML + "Sent: " + messageInput.value + "<br/>";
            resultDiv.scrollTop = resultDiv.scrollHeight;
        };

        var failure = function() {
            alert("Failed writing data to the bluefruit le");
        };

        var data = stringToBytes(messageInput.value);
        var deviceId = event.target.dataset.deviceId;

        if (app.writeWithoutResponse) {
            ble.writeWithoutResponse(
                deviceId,
                bluefruit.serviceUUID,
                bluefruit.txCharacteristic,
                data, success, failure
            );
        } else {
            ble.write(
                deviceId,
                bluefruit.serviceUUID,
                bluefruit.txCharacteristic,
                data, success, failure
            );
        }

    },
    connect: function(event) {
        app.showStatusText('Connecting All');
        //var deviceId = event.target.dataset.deviceId;
        //ble.disconnect(deviceId, app.showMainPage, app.onError);
        for (var i = 0; i < badges.length; ++i) {
            // careful with te 
            var badge=badges[i];
            app.connectToDevice(badge);
        }
    },
    isConnected: function(event) {
        console.log('isConnected?');
        for (var i = 0; i < badges.length; ++i) {
            var badge=badges[i];
            console.log("Status for "+badge+": "+badgesConnStat[badge]);
            ble.isConnected(
                badge,
                (function(badge) { // careful - create a closure here
                    return function() {
                        console.log("Peripheral "+badge+" is connected");
                    }
                })(badge),
                (function(badge) { // careful - create a closure here
                    return function() {
                        console.log("Peripheral "+badge+" is *not* connected");
                    }
                })(badge)
            );
        }
    },    
    disconnect: function(event) {
        app.showStatusText('Disconnecting All');
        for (var i = 0; i < badges.length; ++i) {
            var badge=badges[i];
            app.disconnectFromDevice(badge);
        }
    },
    connectToDeviceWrap : function(deviceId){
        return function() {
            app.connectToDevice(deviceId);
        }
    },
    disconnectFromDeviceWrap : function(deviceId){
        return function() {
            app.disconnectFromDevice(deviceId);
        }
    },    
    autogo: function(event) {
        app.showStatusText('Starting auto connect and disconnect');
        for (var i = 0; i < badges.length; ++i) {
            var badge=badges[i];
            var f = app.connectToDeviceWrap(badge);
            console.log("Starting timer for connecting to "+badge);
            var interval = setInterval(f,1000);

            var f2 = app.disconnectFromDeviceWrap(badge);
            console.log("Starting timer for disconnecting from "+badge);
            var interval2 = setInterval(f2,1500);            
        }
    },
    showMainPage: function() {
        mainPage.hidden = false;
        detailPage.hidden = true;
    },
    showDetailPage: function() {
        mainPage.hidden = true;
        detailPage.hidden = false;
    },
    onError: function(reason) {
        console.log('Error '+ reason);
        //alert("ERROR: " + reason); // real apps should use notification.alert
    },
    showStatusText: function(info) {
        console.log(info);
        document.getElementById("statusText").innerHTML = info;
    }
    
};
