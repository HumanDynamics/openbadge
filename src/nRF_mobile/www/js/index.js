require('q');
var qbluetoothle = require('./qbluetoothle');
var Badge = require('./badge');
struct = require('./struct.js').struct;

window.LOCALSTORAGE_GROUP_KEY = "groupkey";
window.LOCALSTORAGE_GROUP = "groupjson";

window.BADGE_SCAN_INTERVAL = 9000;
window.BADGE_SCAN_DURATION = 8000;

window.WATCHDOG_SLEEP = 5000;

window.LOG_SYNC_INTERVAL = 30 * 1000;
window.CHART_UPDATE_INTERVAL = 5 * 1000;
window.DEBUG_CHART_WINDOW = 1000 * 60 * 2;

window.CHECK_BLUETOOTH_STATUS_INTERVAL = 5 * 60 * 1000; //how often to just try to enable bluetooth. separate from the warning system.
window.CHECK_MEETING_LENGTH_INTERVAL = 3 * 60 * 60 * 1000;
window.CHECK_MEETING_LENGTH_REACTION_TIME = 5 * 60 * 1000;

BATTERY_YELLOW_THRESHOLD = 2.6;
BATTERY_RED_THRESHOLD = 2.4;

BLUETOOTH_OFF_WARNING_TIMEOUT = 5 * 60 * 1000; // if you haven't see bluetooth in this long, send a warning
BLUETOOTH_OFF_WARNING_INTERVAL = 5 * 1000; // how often to check for bluetooth to give the warning
NO_BADGE_SEEN_WARNING_TIMEOUT = 5 * 60 * 1000; // if you haven't seen a badge in this long, send a warning
NO_BADGE_SEEN_WARNING_INTERVAL = 5 * 1000; // how often to check for badges for the warning

RECORDING_TIMEOUT_MINUTES = 5;

window.SHOW_BADGE_CONSOLE = false;


/***********************************************************************
 * Model Declarations
 */

/**
 * Group Model
 * Holds the data of a group, constructed from json data from the server
 */
function Group(groupJson) {
    this.name = groupJson.name;
    this.key = groupJson.key;
    this.visualization_ranges = groupJson.visualization_ranges;
    this.members = [];
    for (var i = 0; i < groupJson.members.length; i++) {
        this.members.push(new GroupMember(groupJson.members[i]));
    }
}

/**
 * Group Member Model
 * Created by Group, contains a Badge, and has some basic data about the member
 */
function GroupMember(memberJson) {
    this.name = memberJson.name;
    this.key = memberJson.key;
    this.badgeId = memberJson.badge;

    this.badge = new Badge.Badge(this.badgeId);
    this.dataAnalyzer = new DataAnalyzer();

    this.badge.badgeDialogue.onNewChunk = function(chunk) {
        this.dataAnalyzer.addChunk(chunk);
    }.bind(this);

    this.badge.badgeDialogue.onChunkCompleted = function(chunk) {
        if (chunk.voltage) {
            this.voltage = chunk.voltage;
        }
        if (this.meeting) {
            this.meeting.logChunk(chunk, this);
        }
    }.bind(this);


    this.badge.onConnect = function() {
        if (this.$lastConnect) {
            this.$lastConnect.text(this.badge.lastConnect.toUTCString());
        }
    }.bind(this);

    this.clearData = function() {
        this.dataAnalyzer.clearData();
        this.badge.badgeDialogue.clearData();
        this.badge.lastConnect = new Date();
        this.seenWarningGiven = false;
    }.bind(this);
}

/**
 * Meeting Model
 * 
 * This is in charge of logging all data to the log file
 */
function Meeting(group, members, type, moderator, description, location) {
    this.members = members;
    this.group = group;
    this.type = type;
    this.location = location;
    this.startTime = new Date();
    this.moderator = moderator;
    this.description = description;
    this.uuid = group.key + "_" + this.startTime.getTime();

    this.showVisualization = function() {
        var now = new Date().getTime() / 1000;
        var ranges = group.visualization_ranges;
        for (var i = 0; i < ranges.length; i++) {
            if (now >= ranges[i].start && now <= ranges[i].end) {
                return true;
            }
        }
        return false;
    }();

    var meeting = this;

    $.each(this.members, function(index, member) {

        member.clearData();

    }.bind(this));

    this.logChunk = function(chunk, member) {

        var chunkData = chunk.toDict(member);

        this.writeLog(JSON.stringify(chunkData));

    }.bind(this);

    this.getLogName = function() {
        return this.uuid + ".txt";
    }.bind(this);

    this.writeLog = function(str) {
        return window.fileStorage.save(this.getLogName(),str + "\n");
    }.bind(this);


    this.printLogFile = function() {
        window.fileStorage.load(this.getLogName()).done(function (data) {
            console.log(data);
        });
    }.bind(this);

    var memberIds = [];
    var memberInitials = [];
    $.each(this.members, function(index, member) {
        memberIds.push(member.key);
        memberInitials.push(getInitials(member.name));
        member.meeting = meeting;
    });
    this.memberKeys = memberIds;
    this.memberInitials = memberInitials;


    this.syncLogFile = function(isComplete, endingMethod) {
        app.syncLogFile(this.getLogName(), !!isComplete, endingMethod, new Date().toJSON());
    }.bind(this);

    var initialData = {
        uuid: this.uuid,
        group: this.group.key,
        members: memberIds,
        startTime: this.startTime.toJSON(),
        moderator: this.moderator,
        location: this.location,
        type: this.type,
        description: this.description.replace(/\s\s+/g, ' '),
        showVisualization: this.showVisualization
    };

    this.writeLog(JSON.stringify(initialData)).done(function() {
        this.syncLogFile(false);
    }.bind(this));

}

/**
 * Abstract Model for a Page. These are representations of any .page element that's a direct child of <body>.
 * Check out the Page Configurations for examples of how to initialize one.
 */
function Page(id, onInit, onShow, onHide, extras) {
    PAGES.push(this);

    this.onPause = function() {};
    this.onResume = function() {};

    if (extras) {
        for (var key in extras) {
            this[key] = extras[key];
            if (typeof this[key] === "function") {
                this[key] = this[key].bind(this);
            }
        }
    }

    this.id = id;
    this.onInit = onInit.bind(this);
    this.onShow = onShow.bind(this);
    this.onHide = onHide.bind(this);
}

// Global list of pages, used for navigation
PAGES = [];


/***********************************************************************
 * Page Configurations
 */


/**
 * Main Page that displays the list of present users for the group
 */
mainPage = new Page("main",
    function onInit() {
        $("#clear-scan-button").click(function() {
            app.clearScannedBadges();
            mainPage.displayActiveBadges();
        });
        $("#settings-button").click(function() {
            app.showPage(settingsPage);
        });
        $("#startMeetingButton").click(function() {

            var activeMembers = 0;
            for (var i = 0; i < app.group.members.length; i++) {
                var member = app.group.members[i];
                if (member.active) {
                    activeMembers += 1;
                }
            }
            if (activeMembers < 2) {
                navigator.notification.alert("Need at least 2 people present to start a meeting.");
                return;
            }
            app.showPage(meetingConfigPage);
        });
        $(".error-retry").click(function() {
            app.refreshGroupData();
        });
    },
    function onShow() {
        app.clearScannedBadges();
        if (app.bluetoothInitialized) {
            // after bluetooth is disabled, it's automatically re-enabled.
            this.beginRefreshData();
            app.disableBluetooth();
        }
    },
    function onHide() {
        clearInterval(app.badgeScanIntervalID);
        app.stopScan();
    },
    {
        onPause: function() {
            this.onHide();
        },
        onResume: function() {
            this.onShow();
        },
        onBluetoothInit: function() {
            this.loadGroupData();

            clearInterval(app.badgeScanIntervalID);
            app.badgeScanIntervalID = setInterval(function() {
                app.scanForBadges();
            }, BADGE_SCAN_INTERVAL);
            app.scanForBadges();
        },
        loadGroupData: function() {

            // load the group from localstorage, if it's saved there.
            var groupJSON = localStorage.getItem(LOCALSTORAGE_GROUP);
            if (groupJSON) {
                try {
                    app.group = new Group(JSON.parse(groupJSON));
                    app.onrefreshGroupDataComplete();
                } catch (e) {
                    app.group = null;
                }
            }
            app.refreshGroupData(! app.group);
        },
        beginRefreshData: function() {
            $(".devicelistMode").addClass("hidden");
            $("#devicelistLoader").removeClass("hidden");
        },
        createGroupUserList: function(invalidkey) {
            $(".devicelistMode").addClass("hidden");

            if (invalidkey) {
                $("#devicelistServerError").removeClass("hidden");
                return;
            }

            if (app.group == null) {
                $("#devicelistError").removeClass("hidden");
                return;
            }

            $("#devicelistContainer").removeClass("hidden");

            $("#devicelist").empty();
            if (! app.group || ! app.group.members) {
                return;
            }
            for (var i = 0; i < app.group.members.length; i++) {
                var member = app.group.members[i];
                $("#devicelist").append($("<li class=\"item\" data-name='{name}' data-device='{badgeId}' data-key='{key}'><span class='name'>{name}</span><i class='icon ion-battery-full battery-icon' /><i class='icon ion-happy present-icon' /></li>".format(member)));
            }

            app.markActiveUsers();
            this.displayActiveBadges();
        },
        displayActiveBadges: function() {

            $("#devicelist .item").removeClass("active");
            $("#devicelist .item .battery-icon").removeClass("red yellow green");
            for (var i = 0; i < app.group.members.length; i++) {
                var member = app.group.members[i];
                if (member.active) {
                    var $el = $("#devicelist .item[data-device='" + member.badgeId + "']");
                    $el.addClass("active");

                    if (member.voltage) {
                        if (member.voltage >= BATTERY_YELLOW_THRESHOLD) {
                            $el.find(".battery-icon").addClass("green");
                        } else if (member.voltage >= BATTERY_RED_THRESHOLD) {
                            $el.find(".battery-icon").addClass("yellow");
                        } else {
                            $el.find(".battery-icon").addClass("red");
                        }
                    }
                }
            }
        },

    }
);

/**
 * Settings Page that lets you save settings
 * @type {Page}
 */
settingsPage = new Page("settings",
    function onInit() {
        $("#saveButton").click(function() {
            localStorage.setItem(LOCALSTORAGE_GROUP_KEY, $("#groupIdField").val());
            app.showPage(mainPage);
            toastr.success("Settings Saved!");
        });
    },
    function onShow() {
        var groupId = localStorage.getItem(LOCALSTORAGE_GROUP_KEY);
        $("#groupIdField").val(groupId);
    },
    function onHide() {
    }
);


/**
 * Meeting Config Page that sets up the meeting before it starts
 * @type {Page}
 */
meetingConfigPage = new Page("meetingConfig",
    function onInit() {
        $("#startMeetingConfirmButton").click(function() {
            var type = $("#meetingTypeField").val();
            var moderator = $("#mediatorField option:selected").data("key");
            var location = $("#meetingLocationField").val();
            var description = $("#meetingDescriptionField").val();
            app.meeting = new Meeting(app.group, this.meetingMembers, type, moderator, description, location);
            app.showPage(meetingPage);
        }.bind(this));
    },
    function onShow() {
        this.meetingMembers = [];
        for (var i = 0; i < app.group.members.length; i++) {
            var member = app.group.members[i];
            if (member.active) {
                this.meetingMembers.push(member);
            }
        }
        var names = [];
        $("#mediatorField").empty();
        $("#mediatorField").append($("<option data-key='none'>None</option>"));
        for (var i = 0; i < this.meetingMembers.length; i++) {
            var member = this.meetingMembers[i];
            names.push(member.name);
            $("#mediatorField").append($("<option data-key='" + member.key + "'>" + member.name + "</option>"));
        }
        $("#memberNameList").text(names.join(", "));
    },
    function onHide() {
    }
);

/**
 * Meeting Page that records data from badges for all members
 * @type {Page}
 */
meetingPage = new Page("meeting",
    function onInit() {
        $("#endMeetingButton").click(function() {
            this.confirmBeforeHide();
        }.bind(this));
        this.$debugCharts = $("#debug-charts");
        $('#debug-chart-button').featherlight(this.$debugCharts, {persist:true});

    },
    function onShow() {
        this.createMemberUserList();

        window.plugins.insomnia.keepAwake();
        app.startAllDeviceRecording();
        app.watchdogStart();
        $("#clock").clock();

        this.timedOut = false;

        clearInterval(this.syncTimeout);
        this.syncTimeout = setInterval(function() {
            app.meeting.syncLogFile();
        }, LOG_SYNC_INTERVAL);

        clearInterval(this.bluetoothCheckTimeout);
        this.bluetoothCheckTimeout = setInterval(function() {
            app.ensureBluetoothEnabled();
        }, CHECK_BLUETOOTH_STATUS_INTERVAL);

        clearInterval(this.memberCheckIntervalID);
        this.memberCheckIntervalID = setInterval(function() {
            this.checkPresentMembers();
        }.bind(this), NO_BADGE_SEEN_WARNING_INTERVAL);


        cordova.plugins.backgroundMode.enable();

        this.initCharts();
        this.setMeetingTimeout();
    },
    function onHide() {
        clearInterval(this.syncTimeout);
        clearInterval(this.chartTimeout);
        clearInterval(this.bluetoothCheckTimeout);
        clearInterval(this.memberCheckIntervalID);
        window.plugins.insomnia.allowSleepAgain();
        app.watchdogEnd();
        app.stopAllDeviceRecording();
        app.meeting.syncLogFile(true, this.timedOut ? "timedout" : "manual");

        cordova.plugins.backgroundMode.disable();

        this.clearMeetingTimeout();
    },
    {
        confirmBeforeHide: function() {
            navigator.notification.confirm("Are you sure?", function(result) {
                if (result == 1) {
                    this.onMeetingComplete();
                }
            }.bind(this));
            return true;
        },
        onBluetoothInit: function() {
            app.watchdogStart();
        },
        timeoutMeeting: function() {
            navigator.vibrate([500,500,500,500,500,500]);//,500,500,500,500,500,100,500,100,500,100,500,100,500,100]);

            navigator.notification.alert("Please press the button to indicate the meeting is still going, or we'll end it automatically in five minutes", function(result) {
                navigator.vibrate([]);
                this.setMeetingTimeout();
            }.bind(this), "Are you still there?", "Continue Meeting");

            this.closeTimeout = setTimeout(function() {
                navigator.notification.dismiss();
                this.clearMeetingTimeout();
                this.timedOut = true;
                app.showMainPage();
            }.bind(this), CHECK_MEETING_LENGTH_REACTION_TIME);
        },
        setMeetingTimeout: function() {

            this.clearMeetingTimeout();

            this.meetingTimeout = setTimeout(function() {

                this.timeoutMeeting();

            }.bind(this), CHECK_MEETING_LENGTH_INTERVAL);
        },
        clearMeetingTimeout: function() {
            clearTimeout(this.closeTimeout);
            clearTimeout(this.meetingTimeout);
        },
        checkPresentMembers: function() {
            $.each(app.meeting.members, function(index, member) {
                if (! member.seenWarningGiven) {
                    if (new Date().getTime() - member.badge.lastConnect > NO_BADGE_SEEN_WARNING_TIMEOUT) {
                        navigator.notification.alert("Hmm, it looks like we haven't seen " + member.name + " in a while. Please restart their badge if they're still here.");
                        member.seenWarningGiven = true;
                    }

                }
            });
        },
        initCharts: function() {

            var $charts = this.$debugCharts;

            $charts.empty();
            var template = _.template($("#debug-chart-template").text());
            $.each(app.meeting.members, function(index, member) {
                var $infocard = $(template({key:member.key,name:member.name}));
                $charts.append($infocard);
                member.chart = new DebugChart($infocard.find("canvas"));
                member.$lastConnect = $infocard.find(".last_update");
            });

            clearInterval(this.chartTimeout);
            this.chartTimeout = setInterval(function() {
                meetingPage.updateCharts();
            }, CHART_UPDATE_INTERVAL);

            var $mmVis = $("#meeting-mediator");
            $mmVis.empty();
            this.mm = null;
            if (app.meeting.showVisualization) {
                $("#visualization").removeClass("hidden");
                $("#meetingmemberlist").addClass("hidden");
                this.mm = new MM({participants: app.meeting.memberKeys,
                        names: app.meeting.memberInitials,
                        transitions: 0,
                        turns: []},
                    app.meeting.moderator,
                    $mmVis.width(),
                    $mmVis.height());
                this.mm.render('#meeting-mediator');
            } else {
                $("#meetingmemberlist").removeClass("hidden");
                $("#visualization").addClass("hidden");
            }


        },
        onMeetingComplete: function() {
            app.showPage(mainPage);
        },
        updateCharts: function() {

            this.displayVoltageLevels();

            var turns = [];
            var totalIntervals = 0;

            var end = new Date().getTime();
            var start = end - DEBUG_CHART_WINDOW;
            
            // calculate intervals 
            var intervals = GroupDataAnalyzer(app.meeting.members,start,end);
            
            // update the chart
            $.each(app.meeting.members, function(index, member) {
                // update cutoff and threshold
                member.dataAnalyzer.updateCutoff();
                member.dataAnalyzer.updateMean();
                //member.dataAnalyzer.updateSpeakThreshold();

                var datapoints = filterPeriod(member.dataAnalyzer.getSamples(),start,end);

                member.chart.render(datapoints, intervals[index], start, end);

                turns.push({participant:member.key, turns:intervals[index].length});
                totalIntervals += intervals[index].length;

            }.bind(this));


            $.each(turns, function(index, turn) {
                turn.turns = turn.turns / totalIntervals;
            });

            if (this.mm) {
                this.mm.updateData({
                    participants: app.meeting.memberKeys,
                    names: app.meeting.memberInitials,
                    transitions: 0,
                    turns: turns
                });
            }

        },
        createMemberUserList: function() {
            $("#meetingmemberlist-content").empty();
            for (var i = 0; i < app.meeting.members.length; i++) {
                var member = app.meeting.members[i];
                $("#meetingmemberlist-content").append($("<li class=\"item\" data-name='{name}' data-device='{badgeId}' data-key='{key}'><span class='name'>{name}</span><i class='icon ion-battery-full battery-icon' /></li>".format(member)));
            }

            this.displayVoltageLevels();
        },
        displayVoltageLevels: function() {

            $("#meetingmemberlist-content .item .battery-icon").removeClass("red yellow green");
            for (var i = 0; i < app.meeting.members.length; i++) {
                var member = app.meeting.members[i];
                var $el = $("#meetingmemberlist-content .item[data-device='" + member.badgeId + "']");
                if (member.voltage) {
                    if (member.voltage >= BATTERY_YELLOW_THRESHOLD) {
                        $el.find(".battery-icon").addClass("green");
                    } else if (member.voltage >= BATTERY_RED_THRESHOLD) {
                        $el.find(".battery-icon").addClass("yellow");
                    } else {
                        $el.find(".battery-icon").addClass("red");
                    }
                }
            }
        },

    }
);


/**
 * This is a chart that displays raw volume and speaking interval data, for debug purposes
 */
function DebugChart($canvas) {

    var canvas = $canvas[0];

    var context = canvas.getContext('2d');

    var magnitude = 100;
    
    var margin = 5;
    var height = canvas.height - margin * 2;
    var width = canvas.width - margin * 2;

    function calcY(y) {
        return height - margin - (Math.min(y / magnitude, 1) * height);
    }
    function calcX(x, start, end) {
        return margin + width * ((x - start) / (end - start));
    }

    this.render = function(series, intervals, start, end) {

        context.clearRect(0, 0, canvas.width, canvas.height);

        context.fillStyle="#B2EBF2";
        for (var i = 0; i < intervals.length; i++) {
            var interval = intervals[i];
            var left = calcX(interval.startTime, start, end);
            var right = calcX(interval.endTime, start, end);
            context.fillRect(left, 0, right - left, canvas.height);
        }

        context.strokeStyle = "#00BFA5";
        context.lineWidth = 2;
        context.beginPath();
        for (var i = 0; i < series.length - 1; i++) {
            
            var point = series[i];
            
            var y = calcY(point.volClippedSmooth);
            var x = calcX(point.timestamp, start, end);
            if (i == 0) {
                context.moveTo(x, y);
            } else {
                context.lineTo(x, y);
            }
            x++;
        }
        context.stroke();

        context.strokeStyle = "#FF4500";
        context.lineWidth = 2;
        context.beginPath();
        for (var i = 0; i < series.length - 1; i++) {

            var point = series[i];

            var y = calcY(point.mean);
            var x = calcX(point.timestamp, start, end);
            if (i == 0) {
                context.moveTo(x, y);
            } else {
                context.lineTo(x, y);
            }
            x++;
        }
        context.stroke();

    }

    return this;
}


/***********************************************************************
 * App Navigation Behavior Configurations
 */

/**
 * App is the main brain behind the core functioning of the app.
 * The app should be the place to do the following operations. Do not have such operations in any other class, 
 * and do not have any other operations in app. They belong elsewhere!
 * 
 * + App and Cordova Initializations
 * + Page Navigation
 * + Bluetooth Operations
 * + Network Operations to talk to the server, including log file syncing
 * + Watchdog, the loop that runs during a meeting
 */
app = {
    /**
     * Initializations
     */
    initialize: function() {
        this.initBluetooth();

        cordova.plugins.backgroundMode.setDefaults({title:'OpenBadge Meeting', text:'OpenBadge Meeting in Progress'});

        document.addEventListener("backbutton", function(e) {

            var currentFeatherlight = $.featherlight.current();
            if (currentFeatherlight) {
                e.preventDefault();
                currentFeatherlight.close();
                return;
            }

            if (app.activePage == mainPage) {
                navigator.app.exitApp();
            } else {
                e.preventDefault();
                if (app.activePage.confirmBeforeHide) {
                    app.activePage.confirmBeforeHide();
                } else {
                    app.showPage(mainPage);
                }
            }
        }, false);

        $(".back-button").click(function() {
            if (app.activePage.confirmBeforeHide) {
                app.activePage.confirmBeforeHide();
            } else {
                app.showPage(mainPage);
            }
        });


        for (var i = 0; i < PAGES.length; i++) {
            PAGES[i].onInit();
        }

        var groupId = localStorage.getItem(LOCALSTORAGE_GROUP_KEY);
        if (! groupId) {
            this.showPage(settingsPage);
        } else {
            this.showPage(mainPage);
        }

        document.addEventListener("resume", function onResume() {
            setTimeout(function() {
                app.synchronizeIncompleteLogFiles();
            }, 100);
            app.activePage.onResume();
        }, false);
        setTimeout(function() {
            app.synchronizeIncompleteLogFiles();
        }, 100);


        document.addEventListener("pause", function onPause() {
            app.stopScan();
            app.activePage.onPause();
        }, false);

        clearInterval(app.checkbluetoothinterval);
        app.lastSeenBluetooth = new Date();
        app.checkbluetoothinterval = setInterval(function() {
            app.checkForBluetoothWarning();
        }, BLUETOOTH_OFF_WARNING_INTERVAL);
    },

    /**
     * Bluetooth Functions
     */
    initBluetooth: function() {
        app.bluetoothInitialized = false;

        bluetoothle.initialize(
            app.bluetoothStatusUpdate,
            {request: false,statusReceiver: true}
        );
    },
    bluetoothStatusUpdate: function (obj) {
        console.log('Success');

        // Android v6.0 required requestPermissions. If it's Android < 5.0 there'll
        // be an error, but don't worry about it.
        if (cordova.platformId === 'android') {
            console.log('Asking for permissions');
            bluetoothle.requestPermission(
                function(obj) {
                    console.log('permissions ok');
                    app.ensureBluetoothEnabled();
                    app.bluetoothInitialized = true;
                    app.activePage.onBluetoothInit();
                },
                function(obj) {
                    console.log('permissions err');
                    app.ensureBluetoothEnabled();
                    app.bluetoothInitialized = true;
                    app.activePage.onBluetoothInit();
                }
            );
        }
    },
    ensureBluetoothEnabled: function() {
        bluetoothle.isEnabled(function(status) {
            if (! status.isEnabled) {
                app.watchdogEnd();
                app.enableBluetooth();
            }
        });
    },
    enableBluetooth: function() {
        console.log("Enabling Bluetooth!");
        bluetoothle.enable(function success() {
            // unused
        }, function error() {
            toastr.error("Could not enable Bluetooth! Please enable it manually.");
            console.log("Could not enable bluetooth!");
        });
    },
    disableBluetooth: function() {
        app.watchdogEnd();
        app.stopScan();
        console.log("Disabling Bluetooth!");
        bluetoothle.disable(function success() {
        }, function error() {
        });
    },
    checkForBluetoothWarning: function() {
        bluetoothle.isEnabled(function(status) {
            if (status.isEnabled) {
                app.lastSeenBluetooth = new Date();
                app.warnedAboutBluetooth = false;
            } else {
                if (!app.warnedAboutBluetooth && new Date().getTime() - app.lastSeenBluetooth.getTime() > BLUETOOTH_OFF_WARNING_TIMEOUT) {
                    navigator.notification.alert("Hmm, it looks like we're unable to turn your Bluetooth on from our app. It may be broken. Please enable it or try to restart your phone. Sorry about this!");
                    app.warnedAboutBluetooth = true;
                }
            }
        });
    },

    /**
     * Log file synchronization functions
     */
    getLogFiles: function (callback) {
        window.fileStorage.list("/").done(function (entries) {
            callback(entries);
        });
    },
    getCompletedMeetings: function(callback) {
        $.ajax(BASE_URL + "get_finished_meetings/" + app.group.key + "/", {
            dataType:"json",
            success: function(result) {
                if (result.success) {
                    callback(result.finished_meetings);
                }
            },
            error: function() {
            }
        });

    },
    synchronizeIncompleteLogFiles: function() {
        if (! app.group || ! app.group.key) {
            return;
        }
        app.getCompletedMeetings(function(finished_meetings) {
            var meeting_ids = {};
            for (var i = 0; i < finished_meetings.length; i++) {
                meeting_ids[finished_meetings[i]] = true;
            }
            app.getLogFiles(function(logfiles) {
                for (var i = 0; i < logfiles.length; i++) {
                    var logfilename = logfiles[i].name;
                    if (logfilename.indexOf(app.group.key) == 0 && ! (logfilename.split(".")[0] in meeting_ids)) {
                        app.syncLogFile(logfilename, true, "sync");
                    }
                }
            });
        })
    },
    syncLogFile: function(filename, isComplete, endingMethod, endTime) {
        var fileTransfer = new FileTransfer();
        var uri = encodeURI(BASE_URL + "log_data/");

        var fileURL = cordova.file.externalDataDirectory + filename;

        var options = new FileUploadOptions();
        options.fileKey = "file";
        options.fileName = fileURL.substr(fileURL.lastIndexOf('/') + 1);
        options.mimeType = "text/plain";
        options.headers = {"X-APPKEY": APP_KEY};

        options.params = {
            isComplete:!!isComplete,
        };
        if (endTime) {
            options.params.endTime = endTime;
        }
        if (endingMethod) {
            options.params.endingMethod = endingMethod;
        }


        fileTransfer.upload(fileURL, uri, function win() {
            console.log("Log backed up successfully!");
        }, function fail() {

        }, options);

    },

    /**
     * Functions to refresh the group data from the backend
     */
    refreshGroupData: function(showLoading, callback) {

        var groupId = localStorage.getItem(LOCALSTORAGE_GROUP_KEY);
        if (app.group && groupId && app.group.key.toUpperCase() != groupId.toUpperCase()) {
            app.group = null;
            showLoading = true;
        }

        if (showLoading) {
            app.onrefreshGroupDataStart();
        }


        $.ajax(BASE_URL + "get_group/" + groupId + "/", {
            dataType:"json",
            success: function(result) {
                if (result.success) {
                    app.group = new Group(result.group);
                    localStorage.setItem(LOCALSTORAGE_GROUP, JSON.stringify(result.group));
                    app.onrefreshGroupDataComplete();
                } else {
                    app.onrefreshGroupDataComplete(true);
                }
                if (callback) {
                    callback(result);
                }
            },
            error: function() {
                app.onrefreshGroupDataComplete();
            }
        });

    },
    onrefreshGroupDataStart: function() {
        mainPage.beginRefreshData();
    },
    onrefreshGroupDataComplete: function(invalidkey) {
        mainPage.createGroupUserList(invalidkey);
    },

    /**
     * Badge Scanning to see which badges are present, and get the status of each badge
     */
    scanForBadges: function() {
        app.activeBadges = app.activeBadges || []
        if (app.scanning || ! app.group) {
            return;
        }
        $("#scanning").removeClass("hidden");
        app.scanning = true;
        qbluetoothle.stopScan().then(function() {
            qbluetoothle.startScan().then(
                function(obj){ // success
                    console.log("Scan completed successfully - "+obj.status)
                    app.onScanComplete();
                }, function(obj) { // error
                    console.log("Scan Start error: " + obj.error + " - " + obj.message)
                    app.onScanComplete();
                }, function(obj) { // progress

                    app.activeBadges.push(obj.address);

                    // extract badge data from advertisement
                    var voltage = null;
                    if (obj.name == "BADGE") {
                        var adbytes = bluetoothle.encodedStringToBytes(obj.advertisement);
                        var adStr = bluetoothle.bytesToString(adbytes);
                        var adBadgeData = adStr.substring(18, 26);
                        var adBadgeDataArr = struct.Unpack('<HfBB', adBadgeData);
                        voltage = adBadgeDataArr[1];
                        app.onScanUpdate(obj.address,voltage);
                    }

                });
        });
    },
    onScanComplete: function() {
        app.scanning = false;
        $("#scanning").addClass("hidden");
        if (! app.group) {
            return;
        }
        app.markActiveUsers();
        mainPage.displayActiveBadges();
    },
    onScanUpdate: function(activeBadge,voltage) {
        if (! app.group) {
            return;
        }

        // update members
        for (var i = 0; i < app.group.members.length; i++) {
            var member = app.group.members[i];
            if (activeBadge == member.badgeId) {
                member.active = true;
                member.voltage = voltage;
                mainPage.displayActiveBadges();
                return;
            }
        }
    },
    stopScan: function() {
        app.scanning = false;
        $("#scanning").addClass("hidden");
        qbluetoothle.stopScan();
    },
    markActiveUsers: function() {
        if (! app || ! app.group) {
            return;
        }
        for (var i = 0; i < app.group.members.length; i++) {
            var member = app.group.members[i];
            member.active = !!~app.activeBadges.indexOf(member.badgeId);
        }
    },
    clearScannedBadges: function() {
        app.activeBadges = [];
        app.markActiveUsers();
    },
    getStatusForEachMember: function() {
        if (! app || ! app.group) {
            return;
        }
        $.each(app.group.members, function(index, member) {
            app.getStatusForMember(member);
        });
    },
    getStatusForMember: function(member) {
        if (member.active) {
            member.badge.queryStatus(
                function callback(data) {
                    member.voltage = data.voltage;
                    mainPage.displayActiveBadges();
                },
                function failure() {
                    member.voltage = null;
                    mainPage.displayActiveBadges();
                }
            );
        }
    },
    startAllDeviceRecording: function() {
        if (! app.meeting) {
            return;
        }
        console.log("Starting recording on all meeting badges!");
        for (var i = 0; i < app.meeting.members.length; ++i) {
            var badge = app.meeting.members[i].badge;
            badge.startRecording();
        }
    },
    stopAllDeviceRecording: function() {
        if (! app.meeting) {
            return;
        }
        console.log("Stopping recording on all meeting badges!");
        for (var i = 0; i < app.meeting.members.length; ++i) {
            var badge = app.meeting.members[i].badge;
            badge.stopRecording();
        }
    },


    /**
     * Navigation
     */
    showPage: function(page) {
        if (app.activePage) {
            app.activePage.onHide();
        }
        app.activePage = page;
        $(".page").removeClass("active");
        $("#" + page.id).addClass("active");
        page.onShow();
    },
    showMainPage: function() {
        this.showPage(mainPage);
    },

    /**
     * Watchdog
     * This is the main loop of the meeting, which checks each of the badges for new data
     */
    watchdogStart: function() {
        // console.log("Starting watchdog");
        clearInterval(app.watchdogTimer);
        app.watchdogTimer = setInterval(function(){ app.watchdog() }, WATCHDOG_SLEEP);
    },
    watchdogEnd: function() {
        // console.log("Ending watchdog");
        if (app.watchdogTimer) {
            clearInterval(app.watchdogTimer);
        }
    },
    watchdog: function() {

        if (! app.meeting) {
            return;
        }

        // Iterate over badges
        for (var i = 0; i < app.meeting.members.length; ++i) {
            var badge = app.meeting.members[i].badge;
            badge.recordAndQueryData();
        }
    },
};



/***********************************************************************
 * File System
 * This wraps the filesystem in mutexes and flags. Only access files through this object!
 */

window.fileStorage = {
    locked:false,
    save: function (name, data, deferred) {
        deferred = deferred || $.Deferred();
        if (window.fileStorage.locked) {
            setTimeout(function() {window.fileStorage.save(name, data, deferred)}, 100);
            return deferred.promise();
        }
        window.fileStorage.locked = true;

        var fail = function (error) {
            window.fileStorage.locked = false;
            deferred.reject(error);
        };

        var gotFileSystem = function (fileSystem) {
            fileSystem.getFile(name, {create: true, exclusive: false}, gotFileEntry, fail);
        };

        var gotFileEntry = function (fileEntry) {
            fileEntry.createWriter(gotFileWriter, fail);
        };

        var gotFileWriter = function (writer) {
            writer.onwrite = function () {
                window.fileStorage.locked = false;
                deferred.resolve();
            };
            writer.onerror = fail;
            writer.seek(writer.length);
            writer.write(data);
        }

        window.resolveLocalFileSystemURL(cordova.file.externalDataDirectory, gotFileSystem, fail);
        return deferred.promise();
    },

    load: function (name, deferred) {
        var deferred = deferred || $.Deferred();
        if (window.fileStorage.locked) {
            setTimeout(function() {window.fileStorage.load(name, deferred)}, 100);
            return deferred.promise();
        }
        window.fileStorage.locked = true;

        var fail = function (error) {
            window.fileStorage.locked = false;
            deferred.reject(error);
        };

        var gotFileSystem = function (fileSystem) {
            fileSystem.getFile(name, { create: false, exclusive: false }, gotFileEntry, fail);
        };

        var gotFileEntry = function (fileEntry) {
            fileEntry.file(gotFile, fail);
        };

        var gotFile = function (file) {
            reader = new FileReader();
            reader.onloadend = function (evt) {
                var data = evt.target.result;
                window.fileStorage.locked = false;
                deferred.resolve(data);
            };

            reader.readAsText(file);
        }

        window.resolveLocalFileSystemURL(cordova.file.externalDataDirectory, gotFileSystem, fail);
        return deferred.promise();
    },

    list: function (name, deferred) {
        var deferred = deferred || $.Deferred();
        if (window.fileStorage.locked) {
            setTimeout(function() {window.fileStorage.list(name, deferred)}, 100);
            return deferred.promise();
        }
        window.fileStorage.locked = true;

        var fail = function (error) {
            window.fileStorage.locked = false;
            deferred.reject(error);
        };

        var gotFileSystem = function (fileSystem) {
            var directoryReader = fileSystem.createReader();
            directoryReader.readEntries(function success(entries) {
                window.fileStorage.locked = false;
                deferred.resolve(entries)
            }, fail);
        };

        window.resolveLocalFileSystemURL(cordova.file.externalDataDirectory + name, gotFileSystem, fail);
        return deferred.promise();
    },

    delete: function (name) {
        var deferred = $.Deferred();

        var fail = function (error) {
            deferred.reject(error);
        };

        var gotFileSystem = function (fileSystem) {
            fileSystem.getFile(name, { create: false, exclusive: false }, gotFileEntry, fail);
        };

        var gotFileEntry = function (fileEntry) {
            fileEntry.remove();
        };

        window.resolveLocalFileSystemURI(cordova.file.externalDataDirectory, gotFileSystem, fail);
        return deferred.promise();
    }
};


/********************************
 * Utility Functions and library initializations
 */

function pad(n, width, z) {
    z = z || '0';
    n = n + '';
    return n.length >= width ? n : new Array(width - n.length + 1).join(z) + n;
}

function getInitials(str) {
    return str.replace(/\W*(\w)\w*/g, '$1').toUpperCase();
}

jQuery.fn.extend({
    clock: function () {
        var start = new Date().getTime();
        $.each(this, function() {
            var $this = $(this);
            var clock = this;
            function setText() {
                var now = new Date().getTime();
                var timediff = Math.floor((now - start) / 1000);
                var hours = Math.floor(timediff / 3600);
                var minutes = pad(Math.floor(timediff % 3600 / 60), 2);
                var seconds = pad(Math.floor(timediff % 60), 2);
                $this.text("{0}:{1}:{2}".format(hours, minutes, seconds));
            };
            if (clock.interval) {
                clearInterval(clock.interval);
            }
            clock.interval = setInterval(function() {
                if (this == null) {
                    clearInterval(clock.interval);
                    return;
                }
                setText();
            }.bind(this), 1000);
            setText();
        });
        return this;
    }
});


toastr.options = {
    "closeButton": false,
    "positionClass": "toast-bottom-center",
    "preventDuplicates": true,
    "showDuration": "200",
    "hideDuration": "500",
    "timeOut": "1000",
}

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

$.ajaxSetup({
    beforeSend: function(xhr, settings) {
        xhr.setRequestHeader("X-APPKEY", APP_KEY);
    }
});

document.addEventListener('deviceready', function() {app.initialize() }, false);

