function Chunk() {
    //Represents a chunk
    var maxSamples = 116;

    this.voltage = -1;
    this.ts = -1;
    this.sampleDelay = -1;
    this.samples = [];

    /**
    *Sets the header of the chunk
    *@param voltage the voltage at the time the chunk was recorded
    *@param the timestamp of the chunk
    *@param the sampleDelay of the chunl
    *
    */
    this.setHeader= function(voltage, ts, sampleDelay) {
        this.voltage = voltage;
        this.ts = ts;
        this.sampleDelay = sampleDelay;
    };

    /*
    *@return the voltage of the chunk
    */
    this.getVoltage = function() {
        return this.voltage;                                                                                
    };

    /*
    *@return the timestamp of the chunk
    */
    this.getTimeStamp = function () {
        return this.ts;
    };

    /*
    *@return the sampleDelay of the chunk
    */
    this.getSampleDelay = function () {
        return this.sampleDelay;
    };

    /*
    *@return the samples of this chunk
    */
    this.getSamples = function(){
        return this.samples;
    }

    /*
    *@param newData the byte array that represents more samples
    */
    this.addSamples = function(newData) {

        //this.samples += newData;
        this.samples = this.samples.concat(newData);
        var sampleLength = this.samples.length;

    }

    /*
    *resets a chunk to defaults settngs
    */
    this.reset = function () {
        this.voltage = -1;
        this.ts = -1;
        this.sampleDelay = -1;
        this.samples = [];
    };

    /*
    *@return whether or not the chunk is full
    */
    this.completed = function() {
        return (this.samples.length >= maxSamples);
    }
}



/**
*Represents a badge dialogue for extracting structs
*@param ID the id of the badge
*@param send a function that the class can call to send strings to the badge
*@param log a function the class can call for debug info
*/
function BadgeDialogue(address, send, log) {
    
    this.StatusEnum = {
        STATUS: 1,
        HEADER: 2,
        DATA: 3,
    };

    var struct = require('./struct.js').struct;
    this.send = send;
    this.log = log;
    this.address = address;
    this.status = this.StatusEnum.STATUS; //at first we expect a status update
    this.dataPackets = 0;

    this.workingChunk; //chunk we are currently building
    this.chunks = []; //will store chunks once Received


    /**
    * This function must be called whenever data was sent from the badge
    * data must be a string
    *
    * Holds states, usually expects to recieve a status
    * If unsent data status is Received, will automatically download chunks
    * Chunks stored as chunk objects in chunk array, can be accessed later by getChunks()
    */
    this.onData = function (data) {
        if (data.length == 1) {
            //if it is length one it must be a status update
            this.status = this.StatusEnum.STATUS;
        }

        if (this.status == this.StatusEnum.STATUS) { // || data.length() == 1) { //must overide if data is of length 1
            this.log("Received a status update: "+data);
            if (data == 'n') {
                //need date
                this.syncBadge();
            } else if (data == 'd') {
                this.log("Data available, extracting: ");
                //data ready
                this.status = this.StatusEnum.HEADER;
                this.send('d'); //request data
            } else if (data == 's') {
                this.log("Badge Synced but no new data");
                //no new data, do nothing for now
            } else {
                this.log("Unknown status: " + data);
            }

        } else if (this.status == this.StatusEnum.HEADER) {
            this.log("Received a header: ");
            var header = struct.Unpack('<Lfh',data);

            var d = new Date();
            var seconds = Math.round(d.getTime()/1000);
            if (header[1] > 2 && header[1] < 4) {
                //valid header?, voltage between 2 and 4
                this.log("&nbsp Timestamp " + header[0]);
                this.log("&nbsp Voltage " + header[1]);

                this.status = this.StatusEnum.DATA;
                this.dataPackets = 0;

                this.workingChunk = new Chunk();
                this.workingChunk.setHeader(header[1], header[0], header[2]);
            } else {
                this.log("invalid header");                
            }
        } else if (this.status == this.StatusEnum.DATA) {
            this.dataPackets++;


            //attempt to parse as a header for debug purposes
            var header = struct.Unpack('<Lfh',data);
            var d = new Date();
            var seconds = Math.round(d.getTime()/1000);
            if (header[1] > 2 && header[1] < 4) {
                //valid header?, voltage between 2 and 4
                this.log("probably missed a header");
            }

            //parse as a datapacket
            var sample_arr = struct.Unpack("<" + data.length + "B", data);
            this.workingChunk.addSamples(sample_arr);


            //if (this.dataPackets == 6) { //old way, tested to work
            if (this.workingChunk.completed()) {
                //we finished a chunk
                this.status = this.StatusEnum.HEADER;
                this.chunks.push(this.workingChunk);
                this.log("Added another chunk, storing " + this.chunks.length + " chunks");

            }
        } else {
            //we messed up somewhere
            this.log("Invalid status enum");
            this.status = this.StatusEnum.STATUS;
        }
    };

    /**
    *Asks the badge for its status
    */
    this.checkStatus = function() {
        this.send('s');
    };

    /**
    *Internal to class
    *updates the given badge with correct time
    */
    this.syncBadge = function() {
        //we must update the badge with the appropriate time
        var d = new Date();
        var seconds = Math.round(d.getTime()/1000);
        this.log('Updating with epoch_seconds: ' + seconds);

        var timeString = struct.Pack('<L',[seconds]);
        this.send(timeString);
    };

    /**
    *@returns the array of chunk objects that this badge has extracted
    */
    this.getChunks = function () {
        return this.chunks;
    };
    
}

exports.BadgeDialogue = module.exports = {
	BadgeDialogue: BadgeDialogue
};
