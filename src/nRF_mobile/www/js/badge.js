/*
 *  Badge object. Handles all communication with the badge
 */
var qbluetoothle = require('./qbluetoothle');

var nrf51UART = {
	serviceUUID: '6e400001-b5a3-f393-e0a9-e50e24dcca9e', //
	txCharacteristic: '6e400002-b5a3-f393-e0a9-e50e24dcca9e', // transmit is from the phone's perspective
	rxCharacteristic: '6e400003-b5a3-f393-e0a9-e50e24dcca9e' // receive is from the phone's perspective
};

function Badge(address) {
	this.address = address;
	this.lastActivity = new Date();
	this.lastDisconnect = new Date();

	// Connects to a badge, run discovery, subscribe, etc
	this.connectDialog = function() {
		var params = {
			address: this.address
		};

		var badge = this; // seems to be required for nested callback			
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
						console.log(obj.address + "|Subscription message: " + obj.status + "|Value: " + str);
					} else if (obj.status == "subscribed") {
						console.log(obj.address + "|Subscribed: " + obj.status);
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
	};

	this.connect = function() {
		var address = this.address;
		console.log(address + "|Beginning connection to");
		this.touchLastActivity();

		var params = {
			address: address
		};
		qbluetoothle.connectDevice(params).then(
			function(obj) { // success
				this.touchLastActivity();
				console.log(obj.address + "|Connected: " + obj.status + "|Keys: " + Object.keys(obj));
			},
			function(obj) { // failure
				if (obj.address) {
					this.touchLastActivity();
					console.log(obj.address + "|General error: " + obj.error + " - " + obj.message + "|Keys: " + Object.keys(obj));
				} else {
					// must be an exception
					console.error(obj);
				}
			}
		);
	};

	this.subscribe = function() {
		var address = this.address;
		console.log(address + "|Subscribing");

		var params = {
			address: address
		};
		qbluetoothle.subscribeToDevice(params).then(
			function(obj) { // success
				// shouldn't get called?
				this.touchLastActivity(address);
				console.log(obj.address + "|Subscribed. Not supposed to get here." + obj.status + "|Keys: " + Object.keys(obj));
			},
			function(obj) { // failure
				this.touchLastActivity();
				console.log(obj.address + "|Subscription error: " + obj.error + " - " + obj.message + "|Keys: " + Object.keys(obj));
				this.close(); //disconnection error
			},
			function(obj) { // notification
				this.touchLastActivity();
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
	};

	this.discover = function() {
		var address = this.address;
		console.log(address + "|Starting discovery");

		var params = {
			address: address
		};
		qbluetoothle.discoverDevice(params).then(
			function(obj) { // success
				this.touchLastActivity(address);
				console.log(obj.address + "|Discovery completed");
			},
			function(obj) { // failure
				this.touchLastActivity();
				if (obj.status == "discovered") {
					console.log(obj.address + "|Unexpected discover status: " + obj.status);
				} else {

					console.log(obj.address + "|Discover error: " + obj.error + " - " + obj.message);
				}
				this.close(); //Best practice is to close on connection error. In our case
				//we also want to reconnect afterwards
			}
		);
	};

	this.close = function() {
		var address = this.address;
		console.log(address + "|Beginning close from");
		var params = {
			"address": address
		};
		qbluetoothle.closeDevice(params).then(
			function(obj) { // success
				console.log(obj.address + "|Close completed: " + obj.status + "|Keys: " + Object.keys(obj));
				this.touchLastActivity();
				this.touchLastDisconnect();
			},
			function(obj) { // failure
				console.log(obj.address + "|Close error: " + obj.error + " - " + obj.message + "|Keys: " + Object.keys(obj));
				this.touchLastActivity();
				this.touchLastDisconnect();
			}
		);
	};

	this.sendString = function(stringValue) {
		var address = this.address;
		console.log(address + "|Trying to send data");
		qbluetoothle.writeToDevice(address, stringValue).then(
			function(obj) { // success
				console.log(obj.address + "|Data sent! " + obj.status + "|Keys: " + Object.keys(obj));
				this.touchLastDisconnect();
			},
			function(obj) { // failure
				console.log(obj.address + "|Error sending data: " + obj.error + "|" + obj.message + "|" + " Keys: " + Object.keys(obj));
				this.touchLastDisconnect();
			}
		);
	};

	this.touchLastActivity = function() {
		var address = this.address;
        var d = new Date();
        console.log(address+"|"+"Updating last activity: "+d);
        this.lastActivity = d;
    };

    this.touchLastDisconnect = function() {
    	var address = this.address;
        var d = new Date();
        console.log(address+"|"+"Updating last disconnect: "+d);
        this.lastDisconnect = d;
    }
}

module.exports = {
	Badge: Badge
};