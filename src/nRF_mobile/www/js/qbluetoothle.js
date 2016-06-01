/* A Q wrapper for bluetoothle 
 * References:
 *  https://github.com/randdusing/cordova-plugin-bluetoothle
 *  https://github.com/randdusing/ng-cordova-bluetoothle/blob/master/ng-cordova-bluetoothle.js
 *  https://github.com/randdusing/ng-cordova-bluetoothle/blob/master/example/index.js
 */
var Q = require('q');

var nrf51UART = {
    serviceUUID:      '6e400001-b5a3-f393-e0a9-e50e24dcca9e', // 0x000c?
    txCharacteristic: '6e400002-b5a3-f393-e0a9-e50e24dcca9e', // transmit is from the phone's perspective
    rxCharacteristic: '6e400003-b5a3-f393-e0a9-e50e24dcca9e'  // receive is from the phone's perspective
};

/*
    Connect to a Bluetooth LE device. The app should use a timer to limit the connecting time in case connecting 
    is never successful. Once a device is connected, it may disconnect without user intervention. The original 
    connection callback will be called again and receive an object with status => disconnected. To reconnect to 
    the device, use the reconnect method. If a timeout occurs, the connection attempt should be canceled using
    disconnect(). For simplicity, I recommend just using connect() and close(), don't use reconnect() or disconnect().
    */
function connectDevice(params) {
    var address = params.address;
    //console.log(address + "|Internal call to connect");
    var d = Q.defer();
    var paramsObj = {
        "address": address
    };
    bluetoothle.connect(
        function(obj) { // success
            //console.log(address + "|Internal call to connect - success");
            if (obj.status == "connected") {
                d.resolve(obj);
            } else {
                obj.connectFailed = true;
                d.reject(obj); //sould this work? it might be a delayed error
            }
        },
        function(obj) { // failure function
            d.reject(obj);
        },
        paramsObj);

    return d.promise;
}

function closeDevice(params) {
    var address = params.address;
    var d = Q.defer();
    var paramsObj = {
        "address": address
    };
    bluetoothle.close(
        function(obj) { // success
            d.resolve(obj);
        },
        function(obj) { // failure function
            d.reject(obj);
        },
        paramsObj);

    return d.promise;
}

function disconnectDevice(params) {
    var address = params.address;
    var d = Q.defer();
    var paramsObj = {
        "address": address
    };
    bluetoothle.disconnect(
        function(obj) { // success
            d.resolve(obj);
        },
        function(obj) { // failure function
            d.reject(obj);
        },
        paramsObj);

    return d.promise;
}

function discoverDevice(params) {
    var address = params.address;
    //console.log(address + "|Internal call to discover");
    var d = Q.defer();
    var paramsObj = {"address":address};
    bluetoothle.discover(
        function(obj) { // success
            //console.log(address + "|Internal call to discover - success");
            if (obj.status == "discovered") {
                d.resolve(obj);
            } else {
                d.reject(obj);
            }
        },
        function(obj) { // failure function
            //console.log(address + "|Internal call to discover - failure: " + obj.error + " - " + obj.message + " Keys: " + Object.keys(obj));
            d.reject(obj);
        },
        paramsObj);

    return d.promise;   
}

function subscribeToDevice(params) {
    var address = params.address;
    //console.log(address + "|Internal call to subscribe");    
    var d = Q.defer();    
    var paramsObj = {
        "address":address,
        "service": nrf51UART.serviceUUID,
        "characteristic": nrf51UART.rxCharacteristic,
        "isNotification" : true
    };

    bluetoothle.subscribe(
        function(obj) { // success
            //console.log(address + "|Internal call to subscribe - success");
            d.notify(obj); // notify and not resolve, so code can get notifications
            //d.resolve(obj);
        },
        function(obj) { // failure function
            //console.log(address + "|Internal call to subscribe - error: " + obj.error + " - " + obj.message + " Keys: " + Object.keys(obj));
            d.reject(obj);
        },
        paramsObj);
    return d.promise; 
}

function writeToDevice(address,strValue) {
    var d = Q.defer();    

    var bytes = bluetoothle.stringToBytes(strValue);
    var encodedString = bluetoothle.bytesToEncodedString(bytes);
    var paramsObj = {
        "address":address,
        "service": nrf51UART.serviceUUID,
        "characteristic": nrf51UART.txCharacteristic,
        "value" : encodedString
    };

    bluetoothle.write(
        function(obj) { // success
            d.resolve(obj);
        },
        function(obj) { // failure function
            d.reject(obj);
        },
        paramsObj);
    return d.promise;    
}

function startScan() {
    var deferred = Q.defer();
    var params = {
        services: [],
        allowDuplicates: false,
        scanMode: bluetoothle.SCAN_MODE_LOW_POWER,
        matchMode: bluetoothle.MATCH_MODE_STICKY,
        matchNum: bluetoothle.MATCH_NUM_ONE_ADVERTISEMENT,
        //callbackType: bluetoothle.CALLBACK_TYPE_FIRST_MATCH,
        scanTimeout: BADGE_SCAN_DURATION,
    };

    // setup timeout if requested
    var scanTimer = null;
    if (params.scanTimeout) {
        scanTimer = setTimeout(function() {
            console.log('Stopping scan');
            bluetoothle.stopScan(
                function(obj) {
                    deferred.resolve(obj);
                },
                function(obj) {
                    deferred.reject(obj);
                }
            );
        }, params.scanTimeout);
    }

    console.log('Start scan');

    bluetoothle.startScan(
        function startScanSuccess(obj) {
            if (obj.status == "scanResult") {
                deferred.notify(obj);
            } else if (obj.status == "scanStarted") {} // do nothing. it started ok
            else {
                obj.error = "unhandled state";
                clearTimeout(scanTimer);
                deferred.reject(obj); // unhandled state
            }
        },
        function startScanError(obj) {
            clearTimeout(scanTimer);
            deferred.reject(obj);
        },
        params);

    return deferred.promise;
}
function stopScan() {
    var deferred = Q.defer();
    bluetoothle.stopScan(
        function stopScanSuccess(obj) {
            deferred.resolve(obj);
        },
        function stopScanError(obj) {
            deferred.resolve(obj);
        }
    );

    return deferred.promise;
}

module.exports = {
    startScan: startScan,
    stopScan: stopScan,
    connectDevice: connectDevice,
    disconnectDevice: disconnectDevice,
    discoverDevice: discoverDevice,
    closeDevice: closeDevice,
    subscribeToDevice: subscribeToDevice,
    writeToDevice: writeToDevice
};