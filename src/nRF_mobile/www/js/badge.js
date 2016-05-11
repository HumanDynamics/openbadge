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
				console.log(obj.address + "|Data sent! " + obj.status + "|Value: " + stringValue + "|Keys: " + Object.keys(obj));
            },
			function(obj) { // failure
				console.log(obj.address + "|Error sending data: " + obj.error + "|" + obj.message + "|" + " Keys: " + Object.keys(obj));
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
                console.log(obj.address + "|Data sent! " + obj.status + "|Value: " + stringValue + "|Keys: " + Object.keys(obj));
            },
            function(obj) { // failure
                console.log(obj.address + "|Error sending data: " + obj.error + "|" + obj.message + "|" + " Keys: " + Object.keys(obj));
            }
        ).fin(function() {
            badge.sendingData = false;
            console.log(address + "|Sent string that needs to immediately close");
            badge.close();
        });
        badge.refreshTimeout();
    }.bind(this);


	// Connects to a badge, run discovery, subscribe, etc
	this.connectDialog = function() {

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
        console.log(badge.address + "|Connecting");

        this.refreshTimeout();

        qbluetoothle.connectDevice(params)
			.then(qbluetoothle.discoverDevice)
			.then(qbluetoothle.subscribeToDevice)
			.then(
				function(obj) { // success (of chain). Shouldn't really be called
					console.log(obj.address + "|Success:" + obj.status + "| Keys: " + Object.keys(obj));
                    console.log(obj);
				},
				function(obj) { // failure
					console.log(obj.address + "|General error: " + obj.error + " - " + obj.message + " Keys: " + Object.keys(obj));
                    badge.isConnecting = false;
                    if (window.aBadgeIsConnecting == badge) {
                        window.aBadgeIsConnecting = null;
                    }
                    console.log(obj);
				},
				function(obj) { // notification
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
						console.log(obj.address + "|Subscribed: " + obj.status);
                        badge.isConnecting = false;
                        if (window.aBadgeIsConnecting == badge) {
                            window.aBadgeIsConnecting = null;
                        }

						// start the dialog
						badge.sendStatusRequest();
					} else {
						console.log(obj.address + "|Unexpected Subscribe Status");
					}
				}
			)
			.fin(function() {
            })
			.done(); // wrap things up. notifications will stop here


	}.bind(this);

    /**
     * This timeout should only be hit in the rarest of circumstances.
     * If a badge has low battery, it may fail to connect and not report it
     * This timeout kills all out locks in the cast that happens. It should never interrupt an actual connection.
     */
    this.refreshTimeout = function() {
        clearTimeout(this.connectionTimeout);
        this.connectionTimeout = setTimeout(function() {
            console.log(this.address + "|Connection timed out ");

            /*
            if (this.isConnecting) {
                this.disconnectThenClose();
                return;
            }*/
            this.isConnecting = false;
            if (window.aBadgeIsConnecting == this) {
                window.aBadgeIsConnecting = null;
            }
            this.close();

        }.bind(this), 20000);
    }.bind(this);

    this.disconnectThenClose = function() {
        var badge = this;

        console.log(this.address + "|Manually disconnecting before close");
        bluetoothle.disconnect(function success() {
                setTimeout(function() {
                    badge.isConnecting = false;
                    badge.close();
                }, 10);
            },
            function failure() {
                console.log(this.address + "|Disconnect failed! Trying again!");
                badge.disconnectThenClose();
            }, {address: this.address});
    }.bind(this);

    // Sends a request for status from the badge
	this.sendStatusRequest = function() {
		var address = this.address;
		console.log(address + "|Requesting status: ");
		var s = "s"; //status
		this.sendString(s);
	}.bind(this);

	this.close = function() {
        if (this.sendingData || this.isConnecting) {
            this.refreshTimeout();
            return;
        }
        if (! this.isConnected || this.isDisconnecting) {
            return;
        }
		var address = this.address;
		console.log(address + "|Calling close");

		var badge = this;

        clearTimeout(this.connectionTimeout);
        badge.isDisconnecting = true;

		var params = {
			"address": address
		};
		qbluetoothle.closeDevice(params).then(
			function(obj) { // success
				console.log(obj.address + "|Close completed: " + obj.status + "|Keys: " + Object.keys(obj));
                console.log(obj);
			},
			function(obj) { // failure
				console.log(obj.address + "|Close error: " + obj.error + " - " + obj.message + "|Keys: " + Object.keys(obj));
                console.log(obj);
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

}

module.exports = {
	Badge: Badge
};