function Chunk() {
    //Represents a chunk
    var maxSamples = 114;

    this.ts = -1;
    this.ts_ms = 0;
    this.voltage = -1;
    this.sampleDelay = -1;
    this.numSamples = -1;
    this.samples = [];

    /**
    *Sets the header of the chunk
    *@param the timestamp of the chunk
    *@param the fraction (ms) of timestamp
    *@param voltage the voltage at the time the chunk was recorded
    *@param the sampleDelay of the chunk
    *@param the number of samples in this chunk
    */
        this.setHeader= function(ts, ts_ms, voltage, sampleDelay, numSamples) {
        this.ts = ts;
        this.ts_ms = ts_ms;
        this.voltage = voltage;
        this.sampleDelay = sampleDelay;
        this.numSamples = numSamples;
    }.bind(this);

    /*
    *@return the voltage of the chunk
    */
    this.getVoltage = function() {
        return this.voltage;                                                                                
    }.bind(this);

    /*
    *@return the timestamp of the chunk
    */
    this.getTimeStamp = function () {
        return this.ts;
    }.bind(this);

    /*
    *@return the sampleDelay of the chunk
    */
    this.getSampleDelay = function () {
        return this.sampleDelay;
    }.bind(this);

    /*
    *@return the samples of this chunk
    */
    this.getSamples = function(){
        return this.samples;
    }.bind(this);

    /*
    *@param newData the byte array that represents more samples
    */
    this.addSamples = function(newData) {
        this.samples = this.samples.concat(newData);
        if (this.samples.length > this.numSamples) {
            // error
            console.error("Too many samples in chunk!",sampleLength);
        }

    }.bind(this);

    /*
    *resets a chunk to defaults settngs
    */
    this.reset = function () {
        this.ts = -1;
        this.ts_ms = 0;
        this.voltage = -1;
        this.sampleDelay = -1;
        this.numSamples = -1;
        this.samples = [];
    }.bind(this);


    /*
    *@return whether or not the chunk is full
    */
    this.completed = function() {
        return (this.samples.length >= this.numSamples);
    }.bind(this);

    this.toDict = function () {
        return {
            voltage:this.voltage,
            timestamp:this.ts,
            timestamp_ms:this.ts_ms,
            sampleDelay:this.sampleDelay,
            numSamples:this.numSamples,
            samples:this.samples
        };
    }.bind(this);
}



/**
*Represents a badge dialogue for extracting structs
*@param badge badge object
*/
function BadgeDialogue(badge) {
    this.recordingTimeoutMinutes = 5;

    this.StatusEnum = {
        STATUS: 1,
        START_REC: 2,
        HEADER: 3,
        DATA: 4
    };

    var struct = require('./struct.js').struct;
    this.badge = badge;
    this.status = this.StatusEnum.STATUS; //at first we expect a status update
    this.dataPackets = 0;

    this.workingChunk; //chunk we are currently building
    this.chunks = []; //will store chunks once Received

    this.log = function(str) {
        this.badge.log(str);
    }.bind(this);

    /*
    Call this function between sessions to reset the state machine
     */
    this.resetState = function () {
        this.status = this.StatusEnum.STATUS;
    }.bind(this);

    /**
    * This function must be called whenever data was sent from the badge
    * data must be a string
    *
    * Holds states, usually expects to recieve a status
    * If unsent data status is Received, will automatically download chunks
    * Chunks stored as chunk objects in chunk array, can be accessed later by getChunks()
    */
    this.onData = function (data) {
        if (this.status == this.StatusEnum.STATUS) {
            var status = struct.Unpack('<BBBLHf',data); //clockSet,dataReady,recording,timestamp_sec,timestamp_ms,voltage
            this.log("Received a status update. Timestamp,ms,Voltage,recStatus: "+status[3]+' '+status[4]+' '+status[5]+' '+status[2]);

            // ask to start recording
            this.status = this.StatusEnum.START_REC;
            this.sendStartRecRequest(this.recordingTimeoutMinutes)
        } else if (this.status == this.StatusEnum.START_REC) {
            this.log("Received recording ack");

            //Ask for data
            this.status = this.StatusEnum.HEADER; // expecting a header next
            var requestTs = this.getLastSeenChunkTs();
            this.sendDataRequest(requestTs.seconds,requestTs.ms); //request data

        } else if (this.status == this.StatusEnum.HEADER) {
            this.log("Received a header: ");
            var header = struct.Unpack('<LHfHB',data); //time, fraction time (ms), voltage, sample delay, number of samples

            if (header[2] > 1 && header[2] < 4) {
                //valid header?, voltage between 1 and 4
                this.log("&nbsp Timestamp " + header[0] + "."+header[1]);
                this.log("&nbsp Voltage " + header[2]);

                this.status = this.StatusEnum.DATA; // expecting a data buffer next
                this.dataPackets = 0;

                this.workingChunk = new Chunk();
                this.workingChunk.setHeader(header[0], header[1], header[2], header[3], header[4]);
            } else if (header[1] == 0) {
                this.log("End of data received, disconnecting");
                badge.close();
            } else {
                this.log("invalid header");                
            }
        } else if (this.status == this.StatusEnum.DATA) {
            this.dataPackets++;
            //parse as a datapacket
            var sample_arr = struct.Unpack("<" + data.length + "B", data);
            this.workingChunk.addSamples(sample_arr);

            if (this.workingChunk.completed()) {
                //we finished a chunk
                this.status = this.StatusEnum.HEADER; // expecting a header next
                this.chunks.push(this.workingChunk);
                if (this.onNewChunk) {
                    this.onNewChunk(this.workingChunk);
                }
                this.log("Added another chunk, storing " + this.chunks.length + " chunks");

            }
        } else {
            //we messed up somewhere
            this.log("Invalid status enum");
            this.status = this.StatusEnum.STATUS;
        }
    }.bind(this);

    /**
    *updates the given badge with correct time and asks for status
    */
    this.sendStatusRequest = function() {
        //Set current time
        var d = new Date();
        var now = this.nowAsSecAndMs();
        this.log('Sending status request with epoch_seconds: ' + now.seconds+ ', ms: '+now.ms);

        var timeString = struct.Pack('<cLH',['s',now.seconds,now.ms]);
        this.badge.sendString(timeString);
    }.bind(this);

    /**
    *sends a start recording request
    */
    this.sendStartRecRequest = function(timeoutMinutes) {
        //Set current time
        var d = new Date();
        var now = this.nowAsSecAndMs();
        this.log('Requesting badge to start recording. Epoch_seconds: ' + now.seconds+ ', ms: '+now.ms+", Timeout: "+timeoutMinutes);

        var timeString = struct.Pack('<cLHH',['1',now.seconds,now.ms,timeoutMinutes]);
        this.badge.sendString(timeString);
    }.bind(this);

        /**
    *sends an end recording request
    */
    this.sendEndRecRequestAndClose = function() {
        //Set current time
        var d = new Date();
        var now = this.nowAsSecAndMs();
        this.log('Requesting badge to end recording');

        var timeString = struct.Pack('<c',['0']);
        this.badge.sendStringAndClose(timeString);
    }.bind(this);

    /**
    *send request for data since given date
    */
    this.sendDataRequest = function(ts_seconds,ts_ms) {
        this.log('Requesting data since epoch_seconds: ' + ts_seconds+ ', ms: '+ts_ms);

        var timeString = struct.Pack('<cLH',['r',ts_seconds,ts_ms]);
        this.badge.sendString(timeString);
    }.bind(this);

    /**
    *@returns the array of chunk objects that this badge has extracted
    */
    this.getChunks = function () {
        return this.chunks;
    }.bind(this);

    this.nowAsSecAndMs = function () {
        var d = new Date();
        var seconds = Math.floor(d.getTime()/1000);
        var ms = d.getTime() % 1000;
        return {'seconds':seconds, 'ms':ms};
    }

    /*
    Returns the date of the last seen chunk (or a fake date, if no chunks)
     */
    this.getLastSeenChunkTs = function () {
        if (this.chunks.length > 0) {
            this.log("Found chunk!");
            var c = this.chunks[this.chunks.length-1];
            return {'seconds':c.ts, 'ms':c.ts_ms};
        } else {
            this.log("No stored chunks. Using default date.");
            var now = this.nowAsSecAndMs();
            return {'seconds':now.seconds - 60, 'ms':now.ms};
        }
    }
}

exports.BadgeDialogue = module.exports = {
	BadgeDialogue: BadgeDialogue
};
