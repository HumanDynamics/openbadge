

/**
*	Wrapper around jspack.js to conform better to python struct
*	Always returns strings when packing
*	Takes in strings when unpacking
*
* 	Requires jspack.js in the same folder
*
*	Spec taken from https://github.com/pgriess/node-jspack/blob/master/README
*/

function Struct()
{
	this.jspack = require('./jspack.js').jspack;



	this.bytesToString = function (buffer) {
	    return String.fromCharCode.apply(null, new Uint8Array(buffer));
	};

	this.stringToBytes = function (string) {
	    var array = new Uint8Array(string.length);
	    for (var i = 0, l = string.length; i < l; i++) {
	        array[i] = string.charCodeAt(i);
	    }
	    //return array.buffer;
	    return array;
	};

	this.Pack = function (fmt, values)
	{
		return this.bytesToString(this.jspack.Pack(fmt, values));
	};

	this.PackTo = function (fmt, a, p, values)
	{
		return this.bytesToString(this.jspack.PackTo(fmt, a, p, values));
	};

	this.Unpack = function (fmt, str)
	{
		var bytes = this.stringToBytes(str);
		return this.jspack.Unpack(fmt, bytes, 0);
	};
};

exports.struct = new Struct();