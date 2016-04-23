var Q = require('q');
var qbluetoothle = require('./qbluetoothle');
var Badge = require('./badge');

var badges = [];

var watchdogTimer = null;

app = {
    // Application Constructor
    initialize: function() {
        this.initBluetooth();

        document.addEventListener("backbutton", function(e) {
            e.preventDefault();
            if ($("#main").hasClass("active")) {
                navigator.app.exitApp();
            } else {
                app.showPage("main");
            }
        }, false);

        this.initMainPage();
        this.initSettingsPage();

        app.refreshGroupData();
        this.showPage("main");

    },


    initMainPage: function() {
        app.presentBadges = [];
        $("#settings-button").click(function() {
            app.showPage("settings");
        });
        $(".back-button").click(function() {
            app.showPage("main");
        });
        $("#startMeetingButton").click(function() {
            if (app.presentBadges.length < 2) {
                navigator.notification.alert("Need at least 2 people present to start a meeting.");
                return;
            }

            // TODO: show the dialog before the meeting starts!
        });
        $("#retryDeviceList").click(function() {
            app.refreshGroupData();
        });
        $("#main").on("hidePage", function() {
            clearInterval(app.badgeScanIntervalID);
        });
        $("#main").on("showPage", function() {
            app.badgeScanIntervalID = setInterval(function() {
                app.scanForBadges();
            }, 5000);
            app.scanForBadges();
        });
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
        app.refreshingGroupData = true;
        $("#devicelistError").addClass("hidden");
        $("#devicelistContainer").addClass("hidden");
        $("#devicelistLoader").removeClass("hidden");
        var groupId = localStorage.getItem("groupId");
        firebase.authThen(function() {
            var timeout = setTimeout(function() {
                app.group = null;
                app.createGroupUserList();
            }, 3000);
            firebase.child("groups").once("value", function(groupDataResult) {
                clearTimeout(timeout);
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
            },
            function() {
                clearTimeout(timeout);
                app.group = null;
                app.createGroupUserList();
            });
        });
    },
    createGroupUserList: function() {
        app.refreshingGroupData = false;
        $("#devicelistLoader").addClass("hidden");

        if (app.group == null) {
            $("#devicelistError").removeClass("hidden");
            return;
        }

        $("#devicelistError").addClass("hidden");
        $("#devicelistContainer").removeClass("hidden");

        $("#devicelist").empty();
        if (! app.group || ! app.group.members) {
            return;
        }
        app.group.memberBadges = [];
        for (var i = 0; i < app.group.members.length; i++) {
            var member = app.group.members[i];
            app.group.memberBadges.push(member.badge);
            $("#devicelist").append($("<li class=\"item\" data-device='{badge}'><span class='name'>{name}</span><i class='icon ion-battery-full battery-icon' /><i class='icon ion-happy present-icon' /></li>".format(member)));
        }

        app.scanForBadges();
    },
    displayPresentBadges: function() {

        $("#devicelist .item").removeClass("active");
        for (var i = 0; i < app.presentBadges.length; i++) {
            var device = app.presentBadges[i];
            $("#devicelist .item[data-device='" + device + "']").addClass("active");
        }
    },


    initBluetooth: function() {

        bluetoothle.initialize(
            app.bluetoothInitializeSuccess,
            {request: true,statusReceiver: true}
        );
    },
    bluetoothInitializeSuccess: function (obj) {
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
    scanForBadges: function() {
        if (app.scanning || app.refreshingGroupData) {
            return;
        }
        app.scanning = true;
        app.presentBadges = [];
        qbluetoothle.stopScan().then(function() {
            qbluetoothle.startScan().then(
                function(obj){ // success
                    app.scanning = false;
                    app.displayPresentBadges();
                    console.log("Scan completed successfully - "+obj.status)
                }, function(obj) { // error
                    app.scanning = false;
                    app.displayPresentBadges();
                    console.log("Scan Start error: " + obj.error + " - " + obj.message)
                }, function(obj) { // progress
                    if (~app.group.memberBadges.indexOf(obj.address) && !~app.presentBadges.indexOf(obj.address)) {
                        app.presentBadges.push(obj.address);
                    }
                });
        });
    },

    showDialog: function($dialog) {
        $(".dialog").hide(0);
        $dialog.show();
    },
    hideDialog: function($dialog) {
        $dialog.hide(200);
    },
    showPage: function(id) {
        $(".page.active").trigger("hidePage");
        $(".page").removeClass("active");
        $("#" + id).addClass("active").trigger("showPage");
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
