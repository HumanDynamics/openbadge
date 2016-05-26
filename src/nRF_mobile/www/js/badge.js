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


	// Connects to a badge, run discovery, subscribe, etc
	this.connectDialog = function() {

        this.log("Attempting to connect");

        if (this.lastDisconnect && this.lastDisconnect.getTime() > new Date().getTime() - 500) {
            this.log("Badge was disconnected too recently. Not connecting.");
            return;
        }

        if (window.aBadgeIsConnecting) {
            if (window.aBadgeIsConnecting != this) {
                setTimeout(function() {
                    this.connectDialog();
                }.bind(this), 1000);
            }
            return;
        }

        if (this.isConnected) {
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

        qbluetoothle.connectDevice(params)
			.then(qbluetoothle.discoverDevice)
			.then(qbluetoothle.subscribeToDevice)
			.then(
				function success(obj) { // success (of chain). Shouldn't really be called
                    badge.log("Success:" + obj.status + "| Keys: " + Object.keys(obj));
                    badge.logObject(obj);
				},
				function fail(obj) { // failure

                    if (obj.connectFailed) {
                        badge.log("Connect failed!");
                    }

                    badge.log("General error: " + obj.error + " - " + obj.message + " Keys: " + Object.keys(obj));
                    badge.isConnecting = false;
                    if (window.aBadgeIsConnecting == badge) {
                        window.aBadgeIsConnecting = null;
                    }
                    badge.logObject(obj);
                    if (obj.message && ~obj.message.indexOf("reconnect or close")) {
                        badge._close();
                    }
                    badge.isConnected = false;
				},
				function notify(obj) { // notification
					if (obj.status == "subscribedResult") {
                        badge.isConnecting = false;
                        if (window.aBadgeIsConnecting == badge) {
                            window.aBadgeIsConnecting = null;
                        }
						var bytes = bluetoothle.encodedStringToBytes(obj.value);
						var str = bluetoothle.bytesToString(bytes);
                        badge.refreshTimeout();
						badge.badgeDialogue.onData(str);
					} else if (obj.status == "subscribed") {
						badge.log("Subscribed: " + obj.status);
                        badge.isConnecting = false;
                        if (window.aBadgeIsConnecting == badge) {
                            window.aBadgeIsConnecting = null;
                        }

						// start the dialog
						badge.sendStatusRequest();
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
            this.isConnecting = false;
            this.isConnected = true;
            this.sendingData = false;
            this._close();

        }.bind(this), 10000);
    }.bind(this);

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

    // Sends a request for status from the badge
	this.sendStatusRequest = function() {
		var address = this.address;
        this.log("Requesting status: ");
		var s = "s"; //status
		this.sendString(s);
	}.bind(this);

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


    this.log = function(str) {
        if (SHOW_BADGE_CONSOLE) {
            console.log(this.address + " | " + new Date() + " | " + str);
        }
    }.bind(this);

    this.logObject = function(obj) {
        if (SHOW_BADGE_CONSOLE) {
            console.log(obj);
        }
    }

}

module.exports = {
	Badge: Badge
};