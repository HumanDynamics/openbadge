var Q = require('q');
var qbluetoothle = require('./qbluetoothle');

//var badges = ['E1:C1:21:A2:B2:E0','D1:90:32:2F:F1:4B'];
var badges = ['F2:42:EC:DA:04:35','D1:90:32:2F:F1:4B'];
//var badges = ['E1:C1:21:A2:B2:E0'];
var badgesInfo = {};
var watchdogTimer = null;

var nrf51UART = {
    serviceUUID:      '6e400001-b5a3-f393-e0a9-e50e24dcca9e', // 0x000c?
    txCharacteristic: '6e400002-b5a3-f393-e0a9-e50e24dcca9e', // transmit is from the phone's perspective
    rxCharacteristic: '6e400003-b5a3-f393-e0a9-e50e24dcca9e'  // receive is from the phone's perspective
};

var colors = ['red','blue','#f00','#0f0','#00f'];
// Devices data.
var dataPoints = {};

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
        discoverButton1.addEventListener('touchstart', this.discoverButtonPressed, false);
        subscribeButton1.addEventListener('touchstart', this.subscribeButtonPressed, false);
        
        disconnectButton1.addEventListener('touchstart', this.disconnect1ButtonPressed, false);
        disconnectButton2.addEventListener('touchstart', this.disconnect2ButtonPressed, false);
        watchdogStartButton.addEventListener('touchstart', this.watchdogStart, false); 
        watchdogEndButton.addEventListener('touchstart', this.watchdogEnd, false); 
        stateButton.addEventListener('touchstart', this.stateButtonPressed, false); 
        sendButton.addEventListener('touchstart', this.sendButtonPressed, false);
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

            dataPoints[badges[i]] = [];
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

        // clear canvas
        app.clearCanvas();
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
 
    connectDevice: function(address) {
        console.log(address + "|Beginning connection to");
        app.touchLastActivity(address);

        var params = {address:address};
        qbluetoothle.connectDevice(params).then(
            function(obj) { // success
                app.touchLastActivity(obj.address);
                console.log(obj.address + "|Connected: " + obj.status + " Keys: " + Object.keys(obj));
                //app.discoverDevice(obj);
            },
            function(obj) { // failure
                app.touchLastActivity(obj.address);
                console.log(obj.address + "|Connect error: " + obj.error + " - " + obj.message + " Keys: " + Object.keys(obj));
                app.closeDevice(obj.address); //Best practice is to close on connection error.
            }
        );
    },

    connectButtonPressed1: function() {
        var address = badges[0];
        console.log("will try to connect - "+address);
        app.connectButton(address);
    },
    connectButtonPressed2: function() {
        var address = badges[1];
        console.log("will try to connect - "+address);
        app.connectButton(address);
    },    
    connectButton: function(address) {
        //app.connectDevice(address);

        //
        dataPoints[address] = [];

        var params = {address:address};
        qbluetoothle.connectDevice(params)
        .then(qbluetoothle.discoverDevice)
        .then(function(obj) {
            var string = "1234"; //set fake date by sending >2 bytes
            console.log(obj.address + "|Trying to call send");
            return qbluetoothle.writeToDevice(obj.address,string)}
        )
        .then(function(obj) {
            var string = "f"; //enable stream
            console.log(obj.address + "|Trying to call send");
            return qbluetoothle.writeToDevice(obj.address,string)}
        )
        .then(qbluetoothle.subscribeToDevice)
        .then(
            function(obj) { // success (of chain). Shouldn't really be called
                console.log(obj.address + "|Success:" + obj.status + "| Keys: " + Object.keys(obj));
            },
            function(obj) { // failure
                if (obj.address) {
                    app.touchLastActivity(obj.address);
                    console.log(obj.address + "|General error: " + obj.error + " - " + obj.message + " Keys: " + Object.keys(obj));
                } else {
                    // must be an exception
                    console.error(obj);
                }
            },
            function(obj) { // notification
                app.touchLastActivity(obj.address);
                if (obj.status == "subscribedResult") {
                    var bytes = bluetoothle.encodedStringToBytes(obj.value);
                    //var str = bluetoothle.bytesToString(bytes);
                    //console.log(obj.address + "|Subscription message: " + obj.status + "|Value: " + str);
                    console.log(obj.address + "|Subscription message: " + obj.status + "|Value: " + bytes[0]);
                    app.drawLines(obj.address,[bytes[0]]);

                } else if (obj.status == "subscribed") {
                    console.log(obj.address + "|Subscribed: " + obj.status);
                } else {
                    console.log(obj.address + "|Unexpected Subscribe Status");
                }   
            }
        )
        .fin(function () { // always close conncetion when done
              console.log(address + "|Finished communicating with device. Disconnecing");
              app.closeDevice(address);
        })
        .done(); // wrap things up. notifications will stop here
    },
    clearCanvas: function() {
        console.log("Clearing canvas");
        var canvas = document.getElementById('canvas');
        var context = canvas.getContext('2d');
        context.clearRect(0, 0, canvas.width, canvas.height);
        /*
        context.beginPath();
        context.moveTo(0,0);
        context.lineTo(300,150);
        context.stroke();
        */
    },
    drawLines: function(deviceAddress,dataArray)
    {
        var canvas = document.getElementById('canvas');
        //console.log("Canvas - "+canvas.width+" x "+canvas.height);        
        var context = canvas.getContext('2d');

        thisDeviceDataPoints = dataPoints[deviceAddress];
        
        thisDeviceDataPoints.push(dataArray);
        //console.log("Datapoints - "+thisDeviceDataPoints);
        if (thisDeviceDataPoints.length > canvas.width)
        {
            thisDeviceDataPoints.splice(0, (thisDeviceDataPoints.length - canvas.width));
        }

        var magnitude = 100;

        function calcY(i)
        {
            return canvas.height - ((i * canvas.height) / magnitude);
        }

        function drawLine(deviceDataPoints,offset, color)
        {
            context.strokeStyle = color;
            context.lineWidth = 2;
            context.beginPath();
            context.moveTo(0, calcY(deviceDataPoints[deviceDataPoints.length-1][offset]));
            var x = 1;
            for (var i = deviceDataPoints.length - 2; i >= 0; i--)
            {
                var y = calcY(deviceDataPoints[i][offset]);
                context.lineTo(x, y);
                x++;
            }
            context.stroke();
        }

        context.clearRect(0, 0, canvas.width, canvas.height);
        //for (var key in badges)
        for (var i = 0; i < badges.length; ++i) {
            var key = badges[i];
            if (dataPoints[key].length > 0) {
                drawLine(dataPoints[key],0, colors[i]);
            }
        }
    },
    discoverButtonPressed:function() {
        var address = badges[0];
        app.discoverDevice(address);
    },
    subscribeButtonPressed:function() {
        var address = badges[0];
        app.subscribeToDevice(address);
    },
    subscribeToDevice: function(address) {
        console.log(address + "|Subscribing");

        var params = {address:address};
        qbluetoothle.subscribeToDevice(params).then(
            function(obj) { // success
                // shouldn't get called?
                app.touchLastActivity(address);
                console.log(obj.address + "|Subscribed. Not supposed to get here." + obj.status + "| Keys: " + Object.keys(obj));
            },
            function(obj) { // failure
                app.touchLastActivity(obj.address);
                console.log(obj.address + "|Subscription error: " + obj.error + " - " + obj.message + " Keys: " + Object.keys(obj));
                app.closeDevice(obj.address); //disconnection error
            },
            function(obj) { // notification
                app.touchLastActivity(obj.address);
                if (obj.status == "subscribedResult") {
                    var bytes = bluetoothle.encodedStringToBytes(obj.value);
                    var str = bluetoothle.bytesToString(bytes);
                    console.log(obj.address + "|Subscription message: " + obj.status + "|Value: " + str);
                } else if (obj.status == "subscribed") {
                    console.log(obj.address + "|Subscribed: " + obj.status);
                } else {
                    console.log("Unexpected Subscribe Status");
                }
            }
        );
    },
    discoverDevice: function(address) {
        console.log(address + "|Starting discovery");

        var params = {address:address};
        qbluetoothle.discoverDevice(params).then(
            function(obj) { // success
                app.touchLastActivity(address);
                console.log(obj.address + "|Discovery completed");
                //app.subscribeToDevice(obj);
            },
            function(obj) { // failure
                app.touchLastActivity(obj.address);
                if (obj.status == "discovered") {
                    console.log(obj.address + "|Unexpected discover status: " + obj.status);
                } else {

                    console.log(obj.address + "|Discover error: " + obj.error + " - " + obj.message);
                }
                app.closeDevice(obj.address); //Best practice is to close on connection error. In our case
                //we also want to reconnect afterwards
            }
        );
    },
    disconnect1ButtonPressed: function() {
        var address = badges[0];
        app.closeDevice(address);
    },
    disconnect2ButtonPressed: function() {
        var address = badges[1];
        app.closeDevice(address);
    },
    closeDevice: function(address)
    {
        console.log(address+"|Beginning close from");
        var params = {"address":address};
        qbluetoothle.closeDevice(params).then(
            function(obj) { // success
                console.log(obj.address+"|Close completed: " + obj.status + " Keys: "+Object.keys(obj));
                app.touchLastDisconnect(obj.address);
            },
            function(obj) { // failure
                console.log(obj.address+"|Close error: " +  obj.error + " - " + obj.message + " Keys: "+Object.keys(obj));
                app.touchLastDisconnect(obj.address);
            }
        );
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
    sendButtonPressed: function() {
        var address = badges[0];
        var string = "s";

        console.log(address + "|Trying to send data");
        qbluetoothle.writeToDevice(address, string).then(
            function(obj) { // success
                console.log(obj.address + "|Data sent! " + obj.status + " Keys: " + Object.keys(obj));
                app.touchLastDisconnect(obj.address);
            },
            function(obj) { // failure
                console.log(obj.address + "|Error sending data: " + obj.error + "|" + obj.message + "|" + " Keys: " + Object.keys(obj));
                app.touchLastDisconnect(obj.address);
            }
        );
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