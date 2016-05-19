var Q = require('q');
var qbluetoothle = require('./qbluetoothle');
var Badge = require('./badge');

window.LOCALSTORAGE_GROUP_KEY = "groupkey";
window.LOCALSTORAGE_GROUP = "groupjson";

window.BADGE_SCAN_INTERVAL = 8000;
window.BADGE_SCAN_DURATION = 7000;

window.WATCHDOG_SLEEP = 5000;

window.LOG_SYNC_INTERVAL = 30 * 1000;
window.CHART_UPDATE_INTERVAL = 5 * 1000;
window.DEBUG_CHART_WINDOW = 1000 * 60 * 2;

window.CHECK_BLUETOOTH_STATUS_INTERVAL = 5 * 60 * 1000;
window.CHECK_MEETING_LENGTH_INTERVAL =  2 * 60 * 60 * 1000;
window.CHECK_MEETING_LENGTH_REACTION_TIME = 1 * 60 * 1000;

window.SHOW_BADGE_CONSOLE = false;



/***********************************************************************
 * Model Declarations
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

function GroupMember(memberJson) {
    this.name = memberJson.name;
    this.key = memberJson.key;
    this.badgeId = memberJson.badge;
}

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

    $.each(this.members, function(index, member) {
        member.badge = new Badge.Badge(member.badgeId);
        member.dataAnalyzer = new DataAnalyzer();
        member.badge.badgeDialogue.onNewChunk = function(chunk) {
            var chunkData = chunk.toDict();
            chunkData.badge = member.badgeId;
            chunkData.member = member.key;
            this.writeLog(JSON.stringify(chunkData));
            member.dataAnalyzer.addChunk(chunk);
        }.bind(this);
        member.badge.onDisconnect = function() {
            if (member.$lastDisconnect) {
                member.$lastDisconnect.text(member.badge.lastDisconnect.toUTCString());
            }
        }
        
    }.bind(this));

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
    });
    this.memberKeys = memberIds;
    this.memberInitials = memberInitials;


    this.syncLogFile = function(isComplete) {
        app.syncLogFile(this.getLogName(), !!isComplete, new Date().toJSON());
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

PAGES = [];

function Page(id, onInit, onShow, onHide, extras) {
    PAGES.push(this);
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


/***********************************************************************
 * Page Configurations
 */


/**
 * Main Page that displays the list of present users for the group
 * @type {Page}
 */
mainPage = new Page("main",
    function onInit() {
        this.presentBadges = [];
        $("#settings-button").click(function() {
            app.showPage(settingsPage);
        });
        $("#startMeetingButton").click(function() {
            if (false && app.presentBadges.length < 2) {
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
        this.loadGroupData();
        app.badgeScanIntervalID = setInterval(function() {
            app.scanForBadges();
        }, BADGE_SCAN_INTERVAL);
        app.scanForBadges();
    },
    function onHide() {
        clearInterval(app.badgeScanIntervalID);
        app.stopScan();
    },
    {
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

            this.displayActiveBadges();
        },
        displayActiveBadges: function() {

            app.activeMembers = app.activeMembers || {};

            $("#devicelist .item").removeClass("active");
            for (var i = 0; i < app.group.members.length; i++) {
                var member = app.group.members[i];
                if (member.badgeId in app.activeMembers) {
                    $("#devicelist .item[data-device='" + member.badgeId + "']").addClass("active");
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
        var $this = this;
        $("#endMeetingButton").click(function() {
            navigator.notification.confirm("Are you sure?", function(result) {
                if (result == 1) {
                    $this.onMeetingComplete();
                }
            });
        });
        this.$debugCharts = $("#debug-charts");
        $('#debug-chart-button').featherlight(this.$debugCharts, {persist:true});

    },
    function onShow() {
        window.plugins.insomnia.keepAwake();
        app.watchdogStart();
        $("#clock").clock();
        this.syncTimeout = setInterval(function() {
            app.meeting.syncLogFile();
        }, LOG_SYNC_INTERVAL);

        this.bluetoothCheckTimeout = setInterval(function() {
            app.ensureBluetoothEnabled();
        }, CHECK_BLUETOOTH_STATUS_INTERVAL);



        cordova.plugins.backgroundMode.enable();

        this.initCharts();
        this.setMeetingTimeout();
    },
    function onHide() {
        clearInterval(this.syncTimeout);
        clearInterval(this.chartTimeout);
        clearInterval(this.bluetoothCheckTimeout);
        window.plugins.insomnia.allowSleepAgain();
        app.watchdogEnd();
        app.meeting.syncLogFile(true);

        cordova.plugins.backgroundMode.disable();

        this.clearMeetingTimeout();
    },
    {
        setMeetingTimeout: function() {

            this.clearMeetingTimeout();

            this.meetingTimeout = setTimeout(function() {
                navigator.vibrate([500,500,500,500,500,500,500,500,500,500,500,100,500,100,500,100,500,100,500,100]);

                navigator.notification.alert("Please press the button to indicate the meeting is still going, or we'll end it automatically in one minute", function(result) {
                    navigator.vibrate([]);
                    this.setMeetingTimeout();
                }.bind(this), "Are you still there?", "Continue Meeting");

                this.closeTimeout = setTimeout(function() {
                    navigator.notification.dismiss();
                    this.clearMeetingTimeout();
                    app.showMainPage();
                }.bind(this), CHECK_MEETING_LENGTH_REACTION_TIME);


            }.bind(this), CHECK_MEETING_LENGTH_INTERVAL);
        },
        clearMeetingTimeout: function() {
            clearTimeout(this.closeTimeout);
            clearTimeout(this.meetingTimeout);
        },
        initCharts: function() {

            var $charts = this.$debugCharts;

            $charts.empty();
            var template = _.template($("#debug-chart-template").text());
            $.each(app.meeting.members, function(index, member) {
                var $infocard = $(template({key:member.key,name:member.name}));
                $charts.append($infocard);
                member.chart = new DebugChart($infocard.find("canvas"));
                member.$lastDisconnect = $infocard.find(".last_update");
            });

            clearInterval(this.chartTimeout);
            this.chartTimeout = setInterval(function() {
                meetingPage.updateCharts();
            }, CHART_UPDATE_INTERVAL);

            var $mmVis = $("#meeting-mediator");
            $mmVis.empty();
            this.mm = null;
            if (app.meeting.showVisualization) {
                this.mm = new MM({participants: app.meeting.memberKeys,
                        names: app.meeting.memberInitials,
                        transitions: 0,
                        turns: []},
                    app.meeting.moderator,
                    $mmVis.width(),
                    $mmVis.height());
                this.mm.render('#meeting-mediator');
            }


        },
        onMeetingComplete: function() {
            app.showPage(mainPage);
        },
        updateCharts: function() {

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
                member.dataAnalyzer.updateSpeakThreshold();

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

        }
    }
);


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

    }

    return this;
}


/***********************************************************************
 * App Navigation Behavior Configurations
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
                app.showPage(mainPage);
            }
        }, false);

        $(".back-button").click(function() {
            app.showPage(mainPage);
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
            app.synchronizeIncompleteLogFiles();
        }, false);
        app.synchronizeIncompleteLogFiles();


        document.addEventListener("pause", function onPause() {
            app.stopScan();
        }, false);
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
            if (app.activePage == meetingPage) {
                app.watchdogStart();
            }
        }
    },
    ensureBluetoothEnabled: function(isDisabledCallback, onEnableCallback) {
        bluetoothle.isEnabled(function(status) {
            if (! status.isEnabled) {
                app.watchdogEnd();
                bluetoothle.enable(function success() {
                    // not used!
                }, function error() {
                    toastr.error("Could not enable Bluetooth! Please enable it manually.");
                });
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
                    if (! (logfilename.split(".")[0] in meeting_ids)) {
                        app.syncLogFile(logfilename, true);
                    }
                }
            });
        })
    },
    syncLogFile: function(filename, isComplete, endTime) {
        var fileTransfer = new FileTransfer();
        var uri = encodeURI(BASE_URL + "log_data/");

        var fileURL = cordova.file.externalDataDirectory + filename;

        var options = new FileUploadOptions();
        options.fileKey = "file";
        options.fileName = fileURL.substr(fileURL.lastIndexOf('/') + 1);
        options.mimeType = "text/plain";
        options.headers = {"X-APPKEY": APP_KEY};

        options.params = {
            isComplete:!!isComplete
        };
        if (endTime) {
            options.params.endTime = endTime;
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
     * Functions to get which badges are present
     */
    scanForBadges: function() {
        if (app.scanning || ! app.group) {
            return;
        }
        app.scanning = true;
        var activeBadges = [];
        qbluetoothle.stopScan().then(function() {
            qbluetoothle.startScan().then(
                function(obj){ // success
                    console.log("Scan completed successfully - "+obj.status)
                    app.onScanComplete(activeBadges);
                }, function(obj) { // error
                    console.log("Scan Start error: " + obj.error + " - " + obj.message)
                    app.onScanComplete(activeBadges);
                }, function(obj) { // progress
                    activeBadges.push(obj.address);
                    app.onScanUpdate(obj.address);
                });
        });
    },
    onScanComplete: function(activeBadges) {
        app.scanning = false;
        if (! app.group) {
            return;
        }
        app.activeMembers = {};
        for (var i = 0; i < app.group.members.length; i++) {
            var member = app.group.members[i];
            member.active = !!~activeBadges.indexOf(member.badgeId);
            if (member.active) {
                app.activeMembers[member.badgeId] = true;
            }
        }
        mainPage.displayActiveBadges();
    },
    onScanUpdate: function(activeBadge) {
        if (! app.group) {
            return;
        }
        app.activeMembers = app.activeMembers || {};
        for (var i = 0; i < app.group.members.length; i++) {
            var member = app.group.members[i];
            if (activeBadge == member.badgeId) {
                member.active = true;
                app.activeMembers[member.badgeId] = true;
                mainPage.displayActiveBadges();
                return;
            }
        }
    },
    stopScan: function() {
        app.scanning = false;
        qbluetoothle.stopScan();
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

    watchdogStart: function() {
        // console.log("Starting watchdog");
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
            badge.connectDialog();
        }
    },
};



/***********************************************************************
 * Initialization of various libraries and global prototype overloads
 */


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


window.fileStorage = {
    locked:false,
    save: function (name, data, deferred) {
        var deferred = deferred || $.Deferred();
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
            writer.seek(writer.length)
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