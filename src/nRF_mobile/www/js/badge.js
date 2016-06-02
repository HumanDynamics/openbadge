/*
 *  Badge object. Handles all communication with the badge
 */
var qbluetoothle = require('./qbluetoothle');
var BadgeDialogue = require('./badgeDialogue.js').BadgeDialogue;

var nrf51UART = {
	serviceUUID: '6e400001-b5a3-f393-e0a9-e50e24dcca9e', //
	txCharacteristic: '6e400002-b5a3-f393-e0a9-e50e24dcca9e', // transmit is from the phone's perspective
	rxCharacteristic: '6e400003-b5a3-f393-e0a9-e50e24dcca9e' // receive is from the phone's perspective
};

function Badge(address) {
	this.address = address;

    this.badgeDialogue = new BadgeDialogue(this);


    /**
     * Sends a string to the device, and refreshes the disconnection timeout
     */
    this.sendString = function(stringValue) {
		var address = this.address;
		var badge = this;
        badge.sendingData = true;
		qbluetoothle.writeToDevice(address, stringValue).then(
			function(obj) { // success
                badge.log("Data sent! " + obj.status + "|Value: " + stringValue + "|Keys: " + Object.keys(obj));
            },
			function(obj) { // failure
                badge.log("Error sending data: " + obj.error + "|" + obj.message + "|" + " Keys: " + Object.keys(obj));
			}
		).fin(function() {
            badge.refreshTimeout();
            badge.sendingData = false;
        });
        badge.refreshTimeout();
	}.bind(this);

    /**
     * Sends a string to the device and immediately closes the connection.
     */
    this.sendStringAndClose = function(stringValue) {
        var address = this.address;
        var badge = this;
        badge.sendingData = true;
        qbluetoothle.writeToDevice(address, stringValue).then(
            function(obj) { // success
                badge.log("Data sent! " + obj.status + "|Value: " + stringValue + "|Keys: " + Object.keys(obj));
            },
            function(obj) { // failure
                badge.log("Error sending data: " + obj.error + "|" + obj.message + "|" + " Keys: " + Object.keys(obj));
            }
        ).fin(function() {
            badge.sendingData = false;
            badge.log("Sent string that needs to immediately close");
            badge.close();
        });
        badge.refreshTimeout();
    }.bind(this);

    /**
     * Queries status from the badge
     * Calls the callback with the data received from the dialog.
     */
    this.queryStatus = function(successCallback, failureCallback) {
        this._connect(
            function onConnect() {
                this.badgeDialogue.sendStatusRequest();
            },
            function onData(data) {
                // this._close();
                successCallback(this.badgeDialogue.onStatusReceived(data));
                this.badgeDialogue.sendEndRecRequestAndClose();
            },
            function onFailure() {
                failureCallback();
            }
        );
    }.bind(this);

    /**
     * Send the signal to the badge to start recording
     */
    this.startRecording = function(callback) {
        this._connect(
            function onConnect() {
                this.badgeDialogue.sendStartRecRequest();
            },
            function onData(data) {
                this.badgeDialogue.onRecordingAckReceived(data);
                if (callback) {
                    callback(this);
                }
            }
        );
    }.bind(this);

    /**
     * Send the signal to the badge to stop recording
     */
    this.stopRecording = function() {
        this._connect(
            function onConnect() {
            },
            function onData(data) {
            }
        );
    }.bind(this);

    /**
     * Send a signal to the badge to ask for volume data
     */
    this.queryData = function(callback) {
        this._connect(
            function onConnect() {
                var requestTs = this.badgeDialogue.getLastSeenChunkTs();
                this.badgeDialogue.sendDataRequest(requestTs.seconds,requestTs.ms);
            },
            function onData(data) {
                if (this.badgeDialogue.isHeader(data)) {
                    this.badgeDialogue.onHeaderReceived(data);
                } else {
                    this.badgeDialogue.onDataReceived(data);
                    callback(this);
                }
            }
        );
    }.bind(this);

    /**
     * Starts recording and then immediately queries for data upon response.
     */
    this.recordAndQueryData = function(callback) {
        var requestedData = false;
        var requestedRecording = false;
        this._connect(
            function onConnect() {
                // first, we send a status request to set the remote time
                this.badgeDialogue.sendStatusRequest();
            },
            function onData(data) {
                if (! requestedRecording) {
                    this.badgeDialogue.onStatusReceived(data);
                    // upon our first response, the first thing we want to do is tell the badge to start recording
                    this.badgeDialogue.sendStartRecRequest();

                    requestedRecording = true;
                    return;
                }
                if (! requestedData) {
                    // upon our second response, we should tell it to start recording
                    this.badgeDialogue.onRecordingAckReceived(data);

                    var requestTs = this.badgeDialogue.getLastSeenChunkTs();
                    this.badgeDialogue.sendDataRequest(requestTs.seconds, requestTs.ms);

                    requestedData = true;
                    return;
                }

                // finally, we've requested status and to start recording and to get data. we've received data!
                if (this.badgeDialogue.isHeader(data) && this.badgeDialogue.expectingHeader) {
                    this.badgeDialogue.onHeaderReceived(data);
                } else {
                    this.badgeDialogue.onDataReceived(data);
                    if (callback) {
                        callback(this);
                    }
                }
            }
        );
    }.bind(this);


    /**
     * Badge connections.
     *
     * WARNING! ACHTUNG!
     *
     * This is for internal use only. Do not call this from any other object. Instead, wrap it in a pretty function like the ones above.
     * This function has a LOT of checks to ensure you don't connect to two badges at once or disconnect from a badge while connecting to it, or any other number of things that can go wrong.
     * DO NOT mess with them, or you will break everything.
     */
	this._connect = function(onConnectCallback, onDataCallback, onFailure) {

        // this.log("Attempting to connect");

        if (this.lastDisconnect && this.lastDisconnect.getTime() > new Date().getTime() - 500) {
            this.log("Badge was disconnected too recently. Not connecting.");
            return;
        }


        if (window.aBadgeIsConnecting) {
            if (window.aBadgeIsConnecting != this && ! this.waitingToConnect) {
                // this.log("Looks like something else is connecting. I'm waiting.");
                setTimeout(function() {
                    this.waitingToConnect = false;
                    this._connect(onConnectCallback, onDataCallback, onFailure);
                }.bind(this), 1000);
                this.waitingToConnect = true;
            }
            return;
        }

        if (this.isConnected) {
            this.log("Already connected. Not connecting.");
            return;
        }

        window.aBadgeIsConnecting = this;

        this.isConnected = true;
        this.isConnecting = true;

		var params = {
			address: this.address
		};

		var badge = this;
        badge.log("Connecting");

        this.refreshTimeout();

        function fail(obj) { // failure

            badge.isConnecting = false;
            if (window.aBadgeIsConnecting == badge) {
                window.aBadgeIsConnecting = null;
            }

            if (obj) {
                if (obj.connectFailed) {
                    badge.log("Connect failed!");
                }

                badge.log("General error: " + obj.error + " - " + obj.message + " Keys: " + Object.keys(obj));
                badge.logObject(obj);
                if (obj.message) {
                    if ( ~obj.message.indexOf("reconnect or close") ||  obj.error == "isDisconnected") {
                        badge.isConnecting = false;
                        if (window.aBadgeIsConnecting == this) {
                            window.aBadgeIsConnecting = null;
                        }
                        badge.isConnected = true;
                        badge.sendingData = false;
                        badge._close();
                    }
                    if ( ~obj.message.indexOf("Characteristic") ||  ~obj.message.indexOf("Service")) {
                        // if we get here, we're super sad because bluetooth has screwed up royally.
                        // throw caution to the wind and try restarting it!
                        app.disableBluetooth();
                    }
                }
            }

            badge.isConnected = false;

            if (onFailure && (! obj || ! obj.error == "isDisconnected")) {
                onFailure.bind(badge)();
            }
        }

        qbluetoothle.connectDevice(params)
			.then(qbluetoothle.discoverDevice)
			.then(qbluetoothle.subscribeToDevice)
			.then(
				function success(obj) { // success (of chain). Shouldn't really be called
                    badge.log("Success:" + obj.status + "| Keys: " + Object.keys(obj));
                    badge.logObject(obj);
				},
                fail,
				function notify(obj) { // notification
					if (obj.status == "subscribedResult") {
                        badge.isConnecting = false;
                        if (window.aBadgeIsConnecting == badge) {
                            window.aBadgeIsConnecting = null;
                        }
						var bytes = bluetoothle.encodedStringToBytes(obj.value);
						var str = bluetoothle.bytesToString(bytes);
                        badge.refreshTimeout();

                        onDataCallback.bind(badge)(str);

					} else if (obj.status == "subscribed") {
						badge.log("Subscribed: " + obj.status);
                        badge.isConnecting = false;
                        if (window.aBadgeIsConnecting == badge) {
                            window.aBadgeIsConnecting = null;
                        }
                        badge.lastConnect = new Date();

                        if (typeof(badge.onConnect) == "function") {
                            badge.onConnect();
                        }

                        onConnectCallback.bind(badge)();

					} else {
                        badge.log("Unexpected Subscribe Status");
					}
				}
			)
			.fin(function() {
            });


	}.bind(this);

    /**
     * This timeout should only be hit in the rarest of circumstances.
     * If a badge has low battery, it may fail to connect and not report it
     * This timeout kills all out locks in the cast that happens. It should never interrupt an actual connection.
     */
    this.refreshTimeout = function() {
        clearTimeout(this.connectionTimeout);
        this.connectionTimeout = setTimeout(function() {
            this.log("Connection timed out ");

            /*
            if (this.isConnecting) {
                this.disconnectThenClose();
                return;
            }*/
            this.isConnecting = false;
            if (window.aBadgeIsConnecting == this) {
                window.aBadgeIsConnecting = null;
            }
            this.isConnected = true;
            this.sendingData = false;
            this._close();

        }.bind(this), 10000);
    }.bind(this);

    /**
     * This function manually calls disconnect before closing. It should probably never be needed.
     */
    this.disconnectThenClose = function() {
        var badge = this;

        badge.log("Manually disconnecting before close");
        bluetoothle.disconnect(function success() {
                setTimeout(function() {
                    badge.isConnecting = false;
                    badge.isConnected = true;
                    badge.sendingData = false;
                    badge._close();
                }, 10);
            },
            function failure() {
                badge.log("Disconnect failed! Trying again!");
                badge.disconnectThenClose();
            }, {address: this.address});
    }.bind(this);

    /**
     * Public facing function to close a connection. It will only actually close the connection if a series of checks get passed. If it can't close the connection cleanly it will just fail.
     */
    this.close = function() {

        var badge = this;


        badge.log("Calling close");

        bluetoothle.isConnected(function success(status) {
            if (status.isConnected) {
                badge.log("Found that badge is still connected");
                badge._close();
            }

        }, function failure() {

        }, {address:badge.address});

    }.bind(this);

    /**
     * Internal close function. Don't call it unless you absolutely MUST force close a connection. And if you do, you will most likely break everything.
     *
     * Instead, use close()
     */
	this._close = function() {
        var badge = this;

        var address = this.address;

        if (this.isConnecting) {
            badge.log("Badge is connecting");
            this.refreshTimeout();
            return;
        }
        if (this.sendingData) {
            badge.log("Badge is sending data");
            this.refreshTimeout();
            return;
        }
        if (this.isDisconnecting) {
            badge.log("Disconnect already in progress!");
            return;
        }
        if (! this.isConnected) {
            badge.log("Badge isn't connected. Canceling disconnect.");
            return;
        }

        if (window.aBadgeIsConnecting == badge) {
            window.aBadgeIsConnecting = null;
        }

        badge.log("Looks like close really should be called");

        clearTimeout(this.connectionTimeout);
        badge.isDisconnecting = true;

		var params = {
			"address": address
		};
		qbluetoothle.closeDevice(params).then(
			function(obj) { // success
                badge.log("Close completed: " + obj.status + "|Keys: " + Object.keys(obj));
                badge.logObject(obj);
			},
			function(obj) { // failure
				badge.log("Close error: " + obj.error + " - " + obj.message + "|Keys: " + Object.keys(obj));
                badge.logObject(obj);
			}
		).fin(function() {
            badge.lastDisconnect = new Date();
            badge.isConnected = false;
            badge.isDisconnecting = false;
            if (typeof(badge.onDisconnect) == "function") {
                badge.onDisconnect();
            }
        });
	}.bind(this);


    /**
     * Console logging that can be turned off
     */
    this.log = function(str) {
        if (SHOW_BADGE_CONSOLE) {
            console.log(this.address + " | " + new Date() + " | " + str);
        }
    }.bind(this);

    /**
     * Used to log an object directly to the console, for debugging purposes
     */
    this.logObject = function(obj) {
        if (SHOW_BADGE_CONSOLE) {
            console.log(obj);
        }
    }

}

module.exports = {
	Badge: Badge
};