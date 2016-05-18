// if at least this amount of time happens between a null signal
// and a talk signal, they are considered to have started talking.
var MIN_TALK_LENGTH = 300;
// If we get no signal for this amount of time, consider them no
// longer talking.
var TALK_TIMEOUT = 1500;
// Time length used for storing samples (in ms
var BUFFER_LENGTH = 1000 * 60 * 5; // 5 minutes)
// Prior for cutoff (max loudness that we expect)
var CUTOFF_PROIOR = 20;
// Prior for threshold (speaking threshold, based on our tests)
var SPEAK_THRESHOLD_PRIOR = 10;
// Weight for weighted mean of thresholds
PRIOR_WEIGHT = 0.9;
// Length of intervals (in ms) for loudness comparison. Since time intervals
// might not be the same
var INTERVAL_INCREMENTS = 25;

// do two given time intervals intersect?
function checkIntersect(startA,endA,startB,endB) {
    return (startA <= endB) && (endA >= startB);
}

/*
This function compares volume data from group members and creates speaking
intervals. Expects a members object
 sampleDuration - the time between samples (or, the duration of the sample)
*/
function GroupDataAnalyzer(members,periodStartTime,periodEndTime,sampleDuration) {
    var loudSpeaker = [];
    var turns = [];
    var totalIntervals = 0;

    // init pos indexes for all members
    console.log("Init pos");
    var membersPos = [];
    $.each(members, function (index, member) {
        var pos = -1;
        if (member.dataAnalyzer.getSamples().length > 0) {
            pos = 0;
        }
        membersPos[index] = pos;
    });

    // compare volumes
    for (var d = periodStartTime; d <= periodEndTime ; d.setTime(d.getTime() + INTERVAL_INCREMENTS)) {
        var dEnd = new Date(d.getTime()+INTERVAL_INCREMENTS);

        var newValue = {'timestamp':d,'memberIndex':-1,'vol':-1};
        console.log("Looking to match",dateToString(d));
        // progress pos in all members to point to relevant sample
        // and check if it's the loudest speaker for this interval
        $.each(members, function (index, member) {
            var samples = member.dataAnalyzer.getSamples();
            var pos = membersPos[index];
            if (pos == -1) return; // already reached end of array, move to the next member

            // keep going until you find intersection. also, stop if none was found
            var sampleStart = null;
            var sampleEnd = null;
            while (pos < samples.length) {
                sampleStart = samples[pos].timestamp;
                sampleEnd = new Date(sampleStart.getTime()+sampleDuration-1);

                if (checkIntersect(d,dEnd,sampleStart,sampleEnd)) {
                    console.log(index,"Found a match for ",dateToString(d),"Found!",dateToString(sampleStart));
                    if (samples[pos].volClippedSmooth > newValue.vol) {
                        newValue.memberIndex = index;
                        newValue.vol = samples[pos].volClippedSmooth;
                        console.log(index,"New max volume !",newValue.vol);
                    }
                    break;
                } else if (sampleStart > dEnd) {
                    // this condition handles gaps in data
                    console.log(index,"Not found, breaking");
                    break;
                } else {
                    pos++;
                }
            }
            // store for the next iteration

            membersPos[index] = pos;
        });
        console.log("Pushing",newValue);
        loudSpeaker.push(newValue);
    }

    console.log("List of all time periods:",loudSpeaker.length)
    //$.each(loudSpeaker, function(d,v) {
    //    console.log(dateToString(d),v);
    //});
}

/*
 This class stores samples for a badge and convert them into talking intervals
 */
function DataAnalyzer() {
    var samples = []; // array of samples : timestamp, volume
    var smoother = new SmoothArray(); // array object used for caluclating smooth value
    var cutoff = CUTOFF_PROIOR;
    var speakThreashold = SPEAK_THRESHOLD_PRIOR ;

    this.purgeSamples = function(timestamp) {
        while (samples.length > 0  &&(timestamp - samples[0].timestamp > BUFFER_LENGTH)) {
            //console.log("Removing from samples:",samples[0]);
            samples.shift();
        }
    }.bind(this);

    this.addChunk = function(chunk) {
        var startTime = chunk.ts;
        var sampleDuration = chunk.sampleDelay;
        var samples = chunk.samples;
        var analyzer = this;
        $.each(samples, function(i, volume) {
            analyzer.addSample(volume, startTime * 1000 + sampleDuration * i, sampleDuration);
        });

    }.bind(this);

    // Adds a sample. Code assumes that the timestamp is monotonically increasing
    // Code will only add samples newer than the last timestamp
    this.addSample = function(vol, timestamp, duration) {
        // is it a new sample?
        if (samples.length > 0) {
            var lastTimestamp = samples[samples.length - 1].timestamp;
            if (timestamp <= lastTimestamp) {
                console.log("Skipping existing sample:",dateToString(timestamp));
                return false;
            }
        }

        // remove old objects samples array
        this.purgeSamples(timestamp);

        // clip the sample
        var volClipped = vol > cutoff ? cutoff : vol;

        // smooth it
        var volClippedSmooth = smoother.push(volClipped);

        // and check if it's above the threshold
        var isSpeak = volClippedSmooth > speakThreashold ? 1 : 0;

        // add sample to array
        samples.push({
            'vol': vol,
            'volClipped' : volClipped,
            'volClippedSmooth' : volClippedSmooth,
            'cutoff' : cutoff,
            'speakThreashold' : speakThreashold,
            'isSpeak' : isSpeak,
            'timestamp': timestamp,
            'duration':duration});

        return true;
    }.bind(this);

    // updates the cutoff
    this.updateCutoff = function() {
        if (samples.length == 0) {
            return CUTOFF_PROIOR;
        }

        var m = meanAndStd(samples,function(sample) {return sample.vol});
        cutoff = (CUTOFF_PROIOR*PRIOR_WEIGHT) + (m.mean - 2*m.std)*(1-PRIOR_WEIGHT);
        console.log("Cutoff prior,value,mean and std:",CUTOFF_PROIOR,cutoff,m.mean,m.std);// calc adjusted cutoff (using samples and prior)
    }

    // updates the threshold
    this.updateSpeakThreshold = function() {
        if (samples.length == 0) {
            return SPEAK_THRESHOLD_PRIOR;
        }

        var m = meanAndStd(samples,function(sample) {return sample.volClippedSmooth});
        speakThreashold = (SPEAK_THRESHOLD_PRIOR*PRIOR_WEIGHT) + (m.mean + 2*m.std)*(1-PRIOR_WEIGHT);
        console.log("Speak priotr,threashold, mean and std:",SPEAK_THRESHOLD_PRIOR,speakThreashold,m.mean,m.std);
    }

    /*******************************************************
     Speaking interval generation
     *******************************************************/
    // Returns true if the two samples are "close" enough together
    this.isWithinTalkTimeout = function (curr, next) {
        var diffInMs = next - curr;
        return  diffInMs <= TALK_TIMEOUT;
    }.bind(this);


    // Returns true if talk interval is long enough
    this.isMinTalkLength = function(talkInterval) {
        var talkIntervalInMs = talkInterval.endTime - talkInterval.startTime;
        return talkIntervalInMs >= MIN_TALK_LENGTH;
    };

    // Generates talking intervals using samples collected during the given time period
    this.generateTalkIntervals = function(periodStartTime,periodEndTime) {
        var talkIntervals = []; // startTime, endTime
        function isInPeriod(sample) {
            return sample.timestamp >= periodStartTime && sample.timestamp <= periodEndTime;
        }
        var samplesInPeriod = samples.filter(isInPeriod);
        function isSpeak(sample) {
            return sample.isSpeak == 1;
        }
        var speakSamples = samplesInPeriod.filter(isSpeak);

        i = 0;
        while (i < speakSamples.length) {
            j = i;

            // traverse the array while samples belong to the same talking interval
            while (j < speakSamples.length - 1 && this.isWithinTalkTimeout(speakSamples[j].timestamp, speakSamples[j+1].timestamp))
            {
                j = j + 1
            }

            // new interval ended. Because each sample is duration millisecond long, we add this to the endTime
            var newTalkInterval = {'startTime':speakSamples[i].timestamp, 'endTime':speakSamples[j].timestamp + speakSamples[j].duration};
            if (this.isMinTalkLength(newTalkInterval)) {
                //console.log("Adding: ",newTalkInterval.startTime,".",newTalkInterval.startTime.getMilliseconds()," ",newTalkInterval.endTime,".",newTalkInterval.endTime.getMilliseconds());
                talkIntervals.push(newTalkInterval);
            }

            // set i to hold the next startTime
            i = j + 1
        }
        return {"data":samplesInPeriod, "intervals":talkIntervals};
    }.bind(this);


    this.getSamples = function() {
        return samples;
    };
}

/*
 Class for smoothening the volume signal
 */
function SmoothArray() {
    // Number of samples that will be used for smoothing the samples
    var SAMPLES_SMOOTHING = 5;
    var smoothArray = [];
    var pos = 0;

    this.push = function(vol) {
        // adds the sample to the correct location
        smoothArray[pos] = vol;
        pos = (pos +1) % SAMPLES_SMOOTHING;

        // returns smoothened value
        var sum = smoothArray.reduce(function(a, b) { return a + b; });
        var mean = sum / smoothArray.length;
        return mean;
    };

    this.getSmoothArray = function() {
        return smoothArray;
    }
}

function dateToString(d) {
    var s = d.getFullYear().toString()+"-"+(d.getMonth()).toString()+"-"+d.getDate().toString();
    s = s + " "+d.getHours().toString()+":"+d.getMinutes().toString()+":"+d.getSeconds()+"."+d.getMilliseconds().toString();
    return s;
}

// Calculates an average
function average(data,getValue){
    var sum = data.reduce(function(sum, cell){
        var value = getValue(cell);
        return sum + value;
    }, 0);

    var avg = sum / data.length;
    return avg;
}

function meanAndStd(data,getValue) {
    // calc mean
    var mean = average(data,getValue);

    // calc std
    var diffs = data.map(function(cell){
        var diff = getValue(cell) - mean;
        return diff;
    });

    var squareDiffs = data.map(function(cell){
        var diff = getValue(cell) - mean;
        var sqr = diff * diff;
        return sqr;
    });

    var avgSquareDiff = average(squareDiffs,function(v) {return v});
    var stdDev = Math.sqrt(avgSquareDiff);

    return {
        mean: mean,
        std: stdDev
    };
}