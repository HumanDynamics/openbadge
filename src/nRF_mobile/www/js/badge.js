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
	this.lastActivity = new Date();
	this.lastDisconnect = new Date();


	this.sendString = function(stringValue) {
		var address = this.address;
		var badge = this;
		qbluetoothle.writeToDevice(address, stringValue).then(
			function(obj) { // success
				console.log(obj.address + "|Data sent! " + obj.status + "|Value: " + stringValue + "|Keys: " + Object.keys(obj));

				badge.touchLastActivity();
			},
			function(obj) { // failure
				console.log(obj.address + "|Error sending data: " + obj.error + "|" + obj.message + "|" + " Keys: " + Object.keys(obj));
				badge.touchLastActivity();
			}
		);
	}.bind(this);

    this.sendStringAndClose = function(stringValue) {
        var address = this.address;
        var badge = this;
        qbluetoothle.writeToDevice(address, stringValue).then(
            function(obj) { // success
                console.log(obj.address + "|Data sent! " + obj.status + "|Value: " + stringValue + "|Keys: " + Object.keys(obj));
                badge.touchLastActivity();
                badge.close();
            },
            function(obj) { // failure
                console.log(obj.address + "|Error sending data: " + obj.error + "|" + obj.message + "|" + " Keys: " + Object.keys(obj));
                badge.touchLastActivity();
                badge.close();
            }
        );
    }.bind(this);


    this.badgeDialogue = new BadgeDialogue(this);

	// Connects to a badge, run discovery, subscribe, etc
	this.connectDialog = function() {
		var params = {
			address: this.address
		};

		var badge = this; // because javascript
		this.touchLastActivity();
		qbluetoothle.connectDevice(params)
			.then(qbluetoothle.discoverDevice)
			.then(qbluetoothle.subscribeToDevice)
			.then(
				function(obj) { // success (of chain). Shouldn't really be called
					console.log(obj.address + "|Success:" + obj.status + "| Keys: " + Object.keys(obj));
				},
				function(obj) { // failure
					badge.touchLastActivity();
					console.log(obj.address + "|General error: " + obj.error + " - " + obj.message + " Keys: " + Object.keys(obj));
				},
				function(obj) { // notification
					// For some reason, "this" is the window or app itself (instead of the badge object)
					// Therefore, we are using a badge vairable that points to the right object
					badge.touchLastActivity();
					if (obj.status == "subscribedResult") {
						var bytes = bluetoothle.encodedStringToBytes(obj.value);
						var str = bluetoothle.bytesToString(bytes);
						badge.badgeDialogue.onData(str);
					} else if (obj.status == "subscribed") {
						console.log(obj.address + "|Subscribed: " + obj.status);

						// start the dialog
						badge.sendStatusRequest();
					} else {
						console.log(obj.address + "|Unexpected Subscribe Status");
					}
				}
			)
			.fin(function() { // always close conncetion when done
				console.log(address + "|Finished communicating with device. Disconnecing");
				badge.close();
			})
			.done(); // wrap things up. notifications will stop here
	}.bind(this);

	// Sends a request for status from the badge
	this.sendStatusRequest = function() {
		var address = this.address;
		console.log(address + "|Requesting status: ");
		var s = "s"; //status
		this.sendString(s);
	}.bind(this);

	this.touchLastActivity = function() {
		var address = this.address;
		var d = new Date();
		console.log(address+"|"+"Updating last activity: "+d);
		this.lastActivity = d;
	}.bind(this);

	this.touchLastDisconnect = function() {
		var address = this.address;
		var d = new Date();
		console.log(address+"|"+"Updating last disconnect: "+d);
		this.lastDisconnect = d;

		if (this.onDisconnect) {
			this.onDisconnect();
		}
	}.bind(this);

	/******************************************************************
	 * Lower level commands
	 *******************************************************************/
	this.connect = function() {
		var address = this.address;
		console.log(address + "|Beginning connection to");
		this.touchLastActivity();

		var badge = this;

		var params = {
			address: address
		};
		qbluetoothle.connectDevice(params).then(
			function(obj) { // success
				badge.touchLastActivity();
				console.log(obj.address + "|Connected: " + obj.status + "|Keys: " + Object.keys(obj));
			},
			function(obj) { // failure
				if (obj.address) {
					badge.touchLastActivity();
					console.log(obj.address + "|General error: " + obj.error + " - " + obj.message + "|Keys: " + Object.keys(obj));
				} else {
					// must be an exception
					console.error(obj);
				}
			}
		);
	}.bind(this);

	this.subscribe = function() {
		var address = this.address;
		console.log(address + "|Subscribing");

		var badge = this;

		var params = {
			address: address
		};
		qbluetoothle.subscribeToDevice(params).then(
			function(obj) { // success
				// shouldn't get called?
				badge.touchLastActivity(address);
				console.log(obj.address + "|Subscribed. Not supposed to get here." + obj.status + "|Keys: " + Object.keys(obj));
			},
			function(obj) { // failure
				badge.touchLastActivity();
				console.log(obj.address + "|Subscription error: " + obj.error + " - " + obj.message + "|Keys: " + Object.keys(obj));
				badge.close(); //disconnection error
			},
			function(obj) { // notification
				badge.touchLastActivity();
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
	}.bind(this);

	this.discover = function() {
		var address = this.address;
		console.log(address + "|Starting discovery");

		var badge = this;

		var params = {
			address: address
		};
		qbluetoothle.discoverDevice(params).then(
			function(obj) { // success
				badge.touchLastActivity(address);
				console.log(obj.address + "|Discovery completed");
			},
			function(obj) { // failure
				badge.touchLastActivity();
				if (obj.status == "discovered") {
					console.log(obj.address + "|Unexpected discover status: " + obj.status);
				} else {

					console.log(obj.address + "|Discover error: " + obj.error + " - " + obj.message);
				}
				badge.close(); //Best practice is to close on connection error. In our case
				//we also want to reconnect afterwards
			}
		);
	}.bind(this);

	this.close = function() {
		var address = this.address;
		console.log(address + "|Calling close");

		var badge = this;

		var params = {
			"address": address
		};
		qbluetoothle.closeDevice(params).then(
			function(obj) { // success
				console.log(obj.address + "|Close completed: " + obj.status + "|Keys: " + Object.keys(obj));
				badge.touchLastActivity();
				badge.touchLastDisconnect();
			},
			function(obj) { // failure
				console.log(obj.address + "|Close error: " + obj.error + " - " + obj.message + "|Keys: " + Object.keys(obj));
				badge.touchLastActivity();
				badge.touchLastDisconnect();
			}
		);
	}.bind(this);

}

module.exports = {
	Badge: Badge
};