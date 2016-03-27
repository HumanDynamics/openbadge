/* A Q wrapper for bluetoothle 
 * References:
 *  https://github.com/randdusing/cordova-plugin-bluetoothle
 *  https://github.com/randdusing/ng-cordova-bluetoothle/blob/master/ng-cordova-bluetoothle.js
 *  https://github.com/randdusing/ng-cordova-bluetoothle/blob/master/example/index.js
 */
var Q = require('q');

/*
    Connect to a Bluetooth LE device. The app should use a timer to limit the connecting time in case connecting 
    is never successful. Once a device is connected, it may disconnect without user intervention. The original 
    connection callback will be called again and receive an object with status => disconnected. To reconnect to 
    the device, use the reconnect method. If a timeout occurs, the connection attempt should be canceled using
    disconnect(). For simplicity, I recommend just using connect() and close(), don't use reconnect() or disconnect().
    */
function connectDevice(address) {
    var d = Q.defer();
    var paramsObj = {
        "address": address
    };
    bluetoothle.connect(
        function(obj) { // success
            if (obj.status == "connected") {
                d.resolve(obj);
            } else {
                // Todo - figure out how to handle this
                console.log(obj.address + "|Unexpected disconnected. Not handled at this point");
                //d.reject(obj); sould this work? it might be a delayed error
            }
        },
        function(obj) { // failure function
            d.reject(obj);
        },
        paramsObj);

    return d.promise;
}

function discoverDevice(address) {
    var d = Q.defer();
    var paramsObj = {"address":address};
    bluetoothle.discover(
        function(obj) { // success
            if (obj.status == "discovered") {
                d.resolve(obj);
            } else {
                d.reject(obj);
            }
        },
        function(obj) { // failure function
            d.reject(obj);
        },
        paramsObj);

    return d.promise;   
}

function subscribeToDevice(address) {
    var d = Q.defer();    
    var paramsObj = {
        "address":address,
        "service": nrf51UART.serviceUUID,
        "characteristic": nrf51UART.rxCharacteristic,
        "isNotification" : true
    };


    bluetoothle.subscribe(
        function(obj) { // success
            if (obj.status == "subscribed") {
                // do nothing
            } else { //status == subscribedResult / received data
                d.notify(obj);
            }
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
        scanTimeout: 10000, // 10 seconds
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
    deviceList.innerHTML = ''; // empties the list

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
module.exports = {
    startScan: startScan,
    connectDevice: connectDevice
};