/* A Q wrapper for bluetoothle */
var Q = require('q');

function startScan() {
        var deferred = Q.defer();
        var params = {
          services:[],
          allowDuplicates: false,
          scanMode: bluetoothle.SCAN_MODE_LOW_POWER,
          matchMode: bluetoothle.MATCH_MODE_STICKY,
          matchNum: bluetoothle.MATCH_NUM_ONE_ADVERTISEMENT,
          //callbackType: bluetoothle.CALLBACK_TYPE_FIRST_MATCH,
          scanTimeout: 10000,
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
                if (obj.status == "scanResult") {deferred.notify(obj);}
                else if (obj.status == "scanStarted"){}// do nothing. it started ok
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
    startScan: startScan
};