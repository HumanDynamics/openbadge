var Q = require('q');
var qbluetoothle = require('./qbluetoothle');
var Badge = require('./badge');

//var badges = ['E1:C1:21:A2:B2:E0','D1:90:32:2F:F1:4B'];
var badgeAddresses = ['EC:21:82:A8:0B:59','D1:90:32:2F:F1:4B'];
var badges = [];
//var badges = ['E1:C1:21:A2:B2:E0'];

var watchdogTimer = null;

var app = {
    // Application Constructor
    initialize: function() {
        this.bindEvents();
        this.initBluetooth();

        this.initMainPage();
        this.initSettingsPage();

        this.showPage("main");

    },

    showPage: function(id) {
        $(".page").removeClass("active");
        $("#" + id).addClass("active").trigger("showPage");
    },

    initMainPage: function() {
        $("#settings-button").click(function() {
            app.showPage("settings");
        });
        $(".back-button").click(function() {
            app.showPage("main");
        });
        document.addEventListener("backbutton", function(e) {
            e.preventDefault();
            if ($("#main").hasClass("active")) {
                navigator.app.exitApp();
            } else {
                app.showPage("main");
            }
        }, false);
        app.refreshGroupData();
    },
    initSettingsPage: function() {
        $("#settings").on("showPage", function() {
            var groupId = localStorage.getItem("groupId");
           $("#groupIdField").val(groupId);
        });
        $("#saveButton").click(function() {
            localStorage.setItem("groupId", $("#groupIdField").val());
            app.showPage("main");
            app.refreshGroupData();
            toastr.success("Settings Saved!");
        });
    },
    
    refreshGroupData: function() {
        $("#devicelistContainer").addClass("hidden");
        $("#devicelistLoader").removeClass("hidden");
        var groupId = localStorage.getItem("groupId");
        firebase.authThen(function() {
            firebase.child("groups").once("value", function(groupDataResult) {
                var groupData = groupDataResult.val();
                app.group = null;
                for (var i = 0; i < groupData.length; i++) {
                    var group = groupData[i];
                    if (group.id == groupId) {
                        app.group = group;
                        break;
                    }
                }
                app.createGroupUserList();
            });
        });
    },
    createGroupUserList: function() {
        $("#devicelistContainer").removeClass("hidden");
        $("#devicelistLoader").addClass("hidden");
        app.refreshDeviceList();

        $("#devicelist").empty();
        if (! app.group || ! app.group.members) {
            return;
        }
        for (var i = 0; i < app.group.members.length; i++) {
            var member = app.group.members[i];
            $("#devicelist").append($("<li class=\"item\" data-device='{badge}'>{name}</li>".format(member)));
        }

        app.updateKnownDevices();
    },
    updateKnownDevices: function(foundDevices) {
        app.knownDevices = foundDevices || app.knownDevices;
        app.knownDevices = app.knownDevices || [];

        $("#devicelist .item").removeClass("active");
        for (var i = 0; i < app.knownDevices.length; i++) {
            var device = app.knownDevices[i];
            $("#devicelist .item[data-device='" + device + "']").addClass("active");
        }
    },


    // Bind Event Listeners
    //
    // Bind any events that are required on startup. Common events are:
    // 'load', 'deviceready', 'offline', and 'online'.
    bindEvents: function() {
        /*
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
        */
    },
    initBluetooth: function() {
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
        var foundDevices = [];
        qbluetoothle.startScan().then(
            function(obj){ // success
                console.log("Scan completed successfully - "+obj.status)
            }, function(obj) { // error
                console.log("Scan Start error: " + obj.error + " - " + obj.message)
            }, function(obj) { // progress
                foundDevices.push(obj.address);
                app.updateKnownDevices(foundDevices);
        });

        app.updateKnownDevices(foundDevices);
    },

    connectButtonPressed1: function() {
        var badge = badges[0];
        console.log("will try to connect - "+badge.address);
        app.connectButton(badge);
    },
    connectButtonPressed2: function() {
        var badge = badges[1];
        console.log("will try to connect - "+badge.address);
        app.connectButton(badge);
    },    
    connectButton: function(badge) {
        badge.connectDialog();
    },
    discoverButtonPressed:function() {
        var badge = badges[0];
        badge.discover();
    },
    subscribeButtonPressed:function() {
        var badge = badges[0];
        badge.subscribe();
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
            badge.connect();
        }
    },
    closeDeviceWrap : function(badge){
        return function() {
            badge.close();
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
        var badge = badges[0];
        var s = "s"; //status

        badge.sendString(s);
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
        for (var i = 0; i < badges.length; ++i) {
            var badge = badges[i];
            var activityDatetime = badge.lastActivity;
            var disconnectDatetime = badge.lastDisconnect;
            console.log(badge.address+"|lastActivity: "+activityDatetime+"|lastDisconnect: "+disconnectDatetime);

            var nowDatetimeMs = Date.now();
            var activityDatetimeMs = activityDatetime.getTime();
            var disconnectDatetimeMs = disconnectDatetime.getTime();

            // kill if connecting for too long and/or if no activity (can update when data recieved)
            // call close() just in case
            // next watchdog call should perform connection
            if (nowDatetimeMs - activityDatetimeMs > 15000) {
                console.log(badge.address+"|Should timeout");

                // touch activity and disconnect date to make sure we don't keep calling this
                // and that we are not calling the next function while there's a disconnect hapenning already
                badge.touchLastActivity();
                badge.touchLastDisconnect();

                // close
                var cf = app.closeDeviceWrap(badge);
                cf();

            } else if (nowDatetimeMs - disconnectDatetimeMs > 5000 && disconnectDatetimeMs >= activityDatetimeMs) {
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
                console.log(badge.address+"|Watchdog, Do nothing");
            }
        }
    },
};

toastr.options = {
    "closeButton": false,
    "positionClass": "toast-bottom-center",
    "preventDuplicates": true,
    "showDuration": "200",
    "hideDuration": "500",
    "timeOut": "1000",
}

var firebase = new Firebase("https://openbadge.firebaseio.com");
firebase.authThen = function(callback) {
    if (firebase.getAuth()) {
        callback();
        return;
    }
    firebase.authWithPassword({
        email    : 'openbadge@openbadge.mit.edu',
        password : 'openestofallthebadges'
    }, function(error, authData) {
        if (error) {
            toastr.error("Authentication error!");
        } else {
            callback();
        }
    });
};

if (!String.prototype.format) {
    String.prototype.format = function() {
        var str = this.toString();
        if (!arguments.length)
            return str;
        var args = typeof arguments[0],
            args = (("string" == args || "number" == args) ? arguments : arguments[0]);
        for (arg in args)
            str = str.replace(RegExp("\\{" + arg + "\\}", "gi"), args[arg]);
        return str;
    }
}

document.addEventListener('deviceready', function() {app.initialize() }, false);
