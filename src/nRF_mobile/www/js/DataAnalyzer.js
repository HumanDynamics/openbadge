// if at least this amount of time happens between a null signal
// and a talk signal, they are considered to have started talking.
var MIN_TALK_LENGTH = 751;
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
 This class stores samples for a badge and convert them into talking intervals
 */
function DataAnalyzer() {
    var samples = []; // array of samples : timestamp, volume
    var smoother = new SmoothArray(); // array object used for caluclating smooth value
    var cutoff = CUTOFF_PROIOR;
    var speakThreashold = SPEAK_THRESHOLD_PRIOR;

    this.purgeSamples = function (timestamp) {
        while (samples.length > 0 && (timestamp - samples[0].timestamp > BUFFER_LENGTH)) {
            //console.log("Removing from samples:",samples[0]);
            samples.shift();
        }
    }.bind(this);

    this.addChunk = function (chunk) {
        var startTime = chunk.ts;
        var startTimeMs = chunk.ts_ms; // start time is a combination of timestamp and fractional part (ms)
        var sampleDuration = chunk.sampleDelay;
        var samples = chunk.samples;
        var analyzer = this;
        $.each(samples, function (i, volume) {
            analyzer.addSample(volume, startTime * 1000 + startTimeMs + sampleDuration * i, sampleDuration);
        });

    }.bind(this);

    // Adds a sample. Code assumes that the timestamp is monotonically increasing
    // Code will only add samples newer than the last timestamp
    this.addSample = function (vol, timestamp, duration) {
        // is it a new sample?
        if (samples.length > 0) {
            var lastTimestamp = samples[samples.length - 1].timestamp;
            if (timestamp <= lastTimestamp) {
                console.log("Skipping existing sample: "+timestamp+" "+dateToString(timestamp));
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
            'volRaw': vol,
            'volClipped': volClipped,
            'volClippedSmooth': volClippedSmooth,
            'cutoff': cutoff,
            'speakThreashold': speakThreashold,
            'isSpeak': isSpeak,
            'timestamp': timestamp,
            'duration': duration
        });

        return true;
    }.bind(this);

    // updates the cutoff
    this.updateCutoff = function () {
        if (samples.length == 0) {
            return CUTOFF_PROIOR;
        }

        var m = meanAndStd(samples, function (sample) {
            return sample.volRaw
        });
        cutoff = (CUTOFF_PROIOR * PRIOR_WEIGHT) + (m.mean + 2 * m.std) * (1 - PRIOR_WEIGHT);
        console.log("Cutoff prior,value,mean and std:", CUTOFF_PROIOR, cutoff, m.mean, m.std);// calc adjusted cutoff (using samples and prior)
    }

    // updates the threshold
    this.updateSpeakThreshold = function () {
        if (samples.length == 0) {
            return SPEAK_THRESHOLD_PRIOR;
        }

        var m = meanAndStd(samples, function (sample) {
            return sample.volClippedSmooth
        });
        speakThreashold = (SPEAK_THRESHOLD_PRIOR * PRIOR_WEIGHT) + (m.mean - 2 * m.std) * (1 - PRIOR_WEIGHT);
        console.log("Speak priotr,threashold, mean and std:", SPEAK_THRESHOLD_PRIOR, speakThreashold, m.mean, m.std);
    }

    this.getSamples = function () {
        return samples;
    }.bind(this);
}
/*******************************************************
 Speaking interval generation
*******************************************************/
// Returns true if the two samples are "close" enough together
function isWithinTalkTimeout(curr, next) {
    var diffInMs = next - curr;
    return  diffInMs <= TALK_TIMEOUT;
}


// Returns true if talk interval is long enough
function isMinTalkLength(talkInterval) {
    var talkIntervalInMs = talkInterval.endTime - talkInterval.startTime;
    return talkIntervalInMs >= MIN_TALK_LENGTH;
};

// Generates talking intervals using given samples (assumes samples indicate speaking)
function generateTalkIntervals(speakSamples) {
    var talkIntervals = []; // startTime, endTime
    var i = 0;
    while (i < speakSamples.length) {
        j = i;

        // traverse the array while samples belong to the same talking interval
        while (j < speakSamples.length - 1 && this.isWithinTalkTimeout(speakSamples[j].timestamp, speakSamples[j+1].timestamp))
        {
            j = j + 1
        }

        // new interval ended. Because each sample is duration millisecond long, we add this to the endTime
        var newTalkInterval = {'startTime':speakSamples[i].timestamp, 'endTime': speakSamples[j].timestamp+speakSamples[j].duration};
        //console.log("start time is: ",newTalkInterval.startTime);
        //console.log("Is it min length?: ",dateToString(newTalkInterval.startTime),dateToString(newTalkInterval.endTime));
        if (this.isMinTalkLength(newTalkInterval)) {
            //console.log("Adding: ",dateToString(newTalkInterval.startTime),dateToString(newTalkInterval.endTime));
            talkIntervals.push(newTalkInterval);
        }
        // set i to hold the next startTime
        i = j + 1
    }
    return talkIntervals;
}

//Class for smoothening the volume signal
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
    var dAsDate = new Date(d);
    var s = dAsDate.getFullYear().toString()+"-"+(dAsDate.getMonth()).toString()+"-"+dAsDate.getDate().toString();
    s = s + " "+dAsDate.getHours().toString()+":"+dAsDate.getMinutes().toString()+":"+dAsDate.getSeconds()+"."+dAsDate.getMilliseconds().toString();
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

/*
 This function compares volume data from group members and creates speaking
 intervals. Expects a members object
 */
function GroupDataAnalyzer(members,periodStartTime,periodEndTime) {
    var wonSamples = []; // List of samples in which member was the loudest. Array of arrays.
    var intervals = [];

    // init pos indexes for all members
    console.log("Init pos");
    var membersPos = [];
    $.each(members, function (index, member) {
        var pos = -1;
        if (member.dataAnalyzer.getSamples().length > 0) {
            pos = 0;
        }
        membersPos[index] = pos;
        wonSamples[index] = [];
        intervals[index] = [];
    });

    // compare volumes
    for (var d = periodStartTime; d <= periodEndTime ; d = d + INTERVAL_INCREMENTS) {
        var dStart = d;
        var dEnd = d + INTERVAL_INCREMENTS;

        var newValue = {'timestamp':dStart,'memberIndex':-1,'vol':-1};
        //console.log("Looking to match",dateToString(d));
        // progress pos in all members to point to relevant sample
        // and check if it's the loudest speaker for this interval
        $.each(members, function (index, member) {
            var samples = member.dataAnalyzer.getSamples();
            var pos = membersPos[index];
            if (pos == -1) return; // already reached end of array, move to the next member

            // keep going until you find intersection. also, stop if none was found
            while (pos < samples.length) {
                var sample = samples[pos];
                var sampleStart = sample.timestamp;
                var sampleEnd = sample.timestamp+sample.duration-1;

                if (checkIntersect(d,dEnd,sampleStart,sampleEnd)) {
                    //console.log(index,"Found a match for ",dateToString(d),"Found!",dateToString(sampleStart));
                    // Only use samples that are louder than what we got, AND are
                    // louder than the threshold
                    if (sample.isSpeak && sample.volClippedSmooth > newValue.vol) {
                        newValue.memberIndex = index;
                        newValue.vol = sample.volClippedSmooth;
                        newValue.duration = INTERVAL_INCREMENTS;
                        //console.log(index,"New max volume !",newValue.vol);
                    }
                    break;
                } else if (sampleStart > dEnd) {
                    // this condition handles gaps in data
                    //console.log(index,"Not found, breaking");
                    break;
                } else {
                    pos++;
                }
            }
            // store for the next iteration
            membersPos[index] = pos;
        });

        // add sample to the "winning" member
        if (newValue.memberIndex > -1) {
            //console.log("Pushing", newValue);
            wonSamples[newValue.memberIndex].push(newValue);
        }
    }

    // generate speaking intervals
    $.each(members, function (index, member) {
        var v = generateTalkIntervals(wonSamples[index]);
        console.log("intervals for ",index,"are",v,"had winning samples:",wonSamples[index].length);
        intervals[index] = v;
    });

    return intervals;
}

function filterPeriod(samples,start,end) {
    function isInPeriod(sample) {
        return sample.timestamp >= start && sample.timestamp <= end;
    }

    return samplesInPeriod = samples.filter(isInPeriod);
}