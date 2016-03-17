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
    },
    onDeviceReady: function() {
        app.showStatusText("Ready");
        //setInterval(function(){ app.refreshDeviceList();; }, 5500); // keep scanning
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

        if (device.name == "BADGE") {
            app.showStatusText('Found: '+ device.id);
            //app.connectToDevice(device.id);

            listItem.dataset.deviceId = device.id;
            listItem.innerHTML = html;
            deviceList.appendChild(listItem);
        }
    },
    connectToDevice: function(deviceId) {
        app.showStatusText('Attemping to connect '+ deviceId);
        var onConnect = function(peripheral) {
                app.showStatusText('Connected '+ deviceId);
                //app.determineWriteType(peripheral);

                // subscribe for incoming data
                //ble.startNotification(deviceId, bluefruit.serviceUUID, bluefruit.rxCharacteristic, app.onData, app.onError);
                //sendButton.dataset.deviceId = deviceId;
                //disconnectButton.dataset.deviceId = deviceId;
                //resultDiv.innerHTML = "";
                //app.showDetailPage();
                app.disconnectFromDevice(deviceId);

            },
            onConnectError = function(reason) {
                app.showStatusText('Error Connecting '+ deviceId + " "+reason);
                //app.disconnectFromDevice(deviceId); // just in case  
            };


        ble.connect(deviceId, onConnect, onConnectError);
    },
    disconnectFromDevice: function(deviceId) {
        var onDisconnectError = function(reason) {
                app.showStatusText('Error disconnecting '+ deviceId + " "+reason);
                //app.disconnectFromDevice(deviceId); // just in case  
            },
            onDisconnect = function(e) {
                console.log('Disconnected from '+ deviceId +' '+ e);
                app.showStatusText('Disconnected from '+ deviceId + ' '+ e);
            };

        app.showStatusText('Disconnecting '+ deviceId);
        ble.disconnect(deviceId, onDisconnect, onDisconnectError);
        app.showStatusText('Disconnect call ended '+ deviceId);
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
        app.connectToDevice('D1:90:32:2F:F1:4B');
        app.connectToDevice('E1:C1:21:A2:B2:E0');
        app.showStatusText('Connect call ended');
    },
    isConnected: function(event) {
        app.showStatusText('isConnected?');
        //var deviceId = event.target.dataset.deviceId;
        //ble.disconnect(deviceId, app.showMainPage, app.onError);
        ble.isConnected(
            'D1:90:32:2F:F1:4B',
            function() {
                console.log("Peripheral D1:90:32:2F:F1:4B is connected");
            },
            function() {
                console.log("Peripheral D1:90:32:2F:F1:4B is *not* connected");
            }
        );
        ble.isConnected(
            'E1:C1:21:A2:B2:E0',
            function() {
                console.log("Peripheral E1:C1:21:A2:B2:E0 is connected");
            },
            function() {
                console.log("Peripheral E1:C1:21:A2:B2:E0 is *not* connected");
            }
        );
        app.showStatusText('isConnected? call ended');
    },    
    disconnect: function(event) {
        app.showStatusText('Disconnecting All');
        //var deviceId = event.target.dataset.deviceId;
        //ble.disconnect(deviceId, app.showMainPage, app.onError);
        ble.disconnect('D1:90:32:2F:F1:4B', app.showMainPage, app.onError);
        ble.disconnect('E1:C1:21:A2:B2:E0', app.showMainPage, app.onError);
        app.showStatusText('Disconnect call ended');
    },
    showMainPage: function() {
        mainPage.hidden = false;
        detailPage.hidden = true;
    },
    showDetailPage: function() {
        mainPage.hidden = true;
        detailPage.hidden = false;
    },
    /*
    onConnectError: function(reason) {
        console.log('Error connecting'+ reason);
        app.showStatusText('could not connect because'+ reason);
        //alert("ERROR Connecting: " + reason); // real apps should use notification.alert
    },
    */
    onError: function(reason) {
        console.log('Error '+ reason);
        //alert("ERROR: " + reason); // real apps should use notification.alert
    },

    showStatusText: function(info) {
        console.log(info);
        document.getElementById("statusText").innerHTML = info;
    }
};
