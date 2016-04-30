/*
This class stores samples for a badge and convert them into talking intervals
It uses a moving RMS to determine a threshold. Since multiplication is 
easier than a root operation, we calculate MS and not RMS.
*/

function DataAnalyzer(sampleFreq) {
    this.sampleFreq= sampleFreq;

    // if at least this amount of time happens between a null signal
    // and a talk signal, they are considered to have started talking.
    var MIN_TALK_LENGTH = 300;
    // If we get no signal for this amount of time, consider them no
    // longer talking.
    var TALK_TIMEOUT = 1000;


    var samples = []; // array of samples : timestamp, volume
    var smoother = new SmoothArray(); // array object used for caluclating smooth value
    var rmsThreshold = new RMSThreshold();

    // Adds a sample. Code assumes that the timestamp is monotonically increasing
    // Timestamp is a Date object
    this.addSample = function(vol,timestamp) {
        // remove old objects samples array
        // TODO - implement

        // Calc smoothened volume
        var sv = smoother.push(vol);

        // update RMS with new sample
        rmsThreshold.addSample(sv);

        // speaking indicator is 1 if smoothened volume is above the threshold
        var isSpeak = rmsThreshold.compareToThreshold(sv);

        // add sample to array
        samples.push({'vol': vol, 'sv' : sv, 'isSpeak' : isSpeak,'timestamp': timestamp});
    };

    // Returns true if the two samples are "close" enough together
    this.isWithinTalkTimeout = function (curr, next) {
        var diffInMs = next - curr;
        return  diffInMs <= TALK_TIMEOUT;
    };


    // Returns true if talk interval is long enough
    this.isMinTalkLength = function(talkInterval) {
        var talkIntervalInMs = talkInterval.endTime - talkInterval.startTime;
        return talkIntervalInMs >= MIN_TALK_LENGTH;
    };

    this.generateTalkIntervals = function() {
        var talkIntervals = []; // startTime, endTime

        function isSpeak(sample) {
            return sample.isSpeak == 1;
        }
        var speakSamples = samples.filter(isSpeak);

        i = 0;
        while (i < speakSamples.length) {
            j = i;

            // traverse the array while samples belong to the same talking interval
            while (j < speakSamples.length - 1 && this.isWithinTalkTimeout(speakSamples[j].timestamp, speakSamples[j+1].timestamp))
            {
                j = j + 1
            }

            // new interval ended. Because each sample is sampleFreq millisecond long, we add this to the endTime
            var newTalkInterval = {'startTime':speakSamples[i].timestamp, 'endTime':new Date(speakSamples[j].timestamp.getTime() + this.sampleFreq)};
            if (!this.isMinTalkLength(newTalkInterval)) {
                console.log("Not adding (too short): " + newTalkInterval.startTime + " " + newTalkInterval.endTime);
            } else {
                console.log("Adding: ",newTalkInterval.startTime,".",newTalkInterval.startTime.getMilliseconds()," ",newTalkInterval.endTime,".",newTalkInterval.endTime.getMilliseconds());
                talkIntervals.push();
            }

            // set i to hold the next startTime
            i = j + 1
        }
        return talkIntervals;
    };


    this.getSamples = function() {
        return samples;
    };

    this.getSamplesSmooth = function() {
        return samplesSmooth;
    }
}

// Class for handling the threshold
function RMSThreshold() {
    var samples = []; // samples used for thresholding
    var meanSquaredThreshold = 0; // (R)MS threshold. It's a sum of squared volumes
    var meanSquaredThresholdCount = 0; // number of elements in the sum. required for calcualting the mean

    this.addSample = function(v,timestamp) {
        // Todo - remove obsolete samples

        // adding sample and updating the threshold
        samples.push({'v': v, 'timestamp': timestamp});
        meanSquaredThreshold = meanSquaredThreshold + (v*v);
        meanSquaredThresholdCount++;
    };

    this.compareToThreshold = function(v) {
        return v*v > meanSquaredThreshold/meanSquaredThresholdCount ? 1 : 0;
    }

}

// Class for smoothening the volume signal
function SmoothArray() {
    // Number of samples that will be used for smoothing the samples
    var SAMPLES_SMOOTHING = 5;
    var smoothArray = [];
    var pos = 0;

    this.push = function(vol) {
        // adds the sample to the correct location
        console.log("Adding "+vol+" to array size "+smoothArray.length);
        smoothArray[pos] = vol;
        pos = (pos +1) % SAMPLES_SMOOTHING;
        console.log("Pos is now "+pos+" to array size "+smoothArray.length);

        // returns smoothened value
        var sum = smoothArray.reduce(function(a, b) { return a + b; });
        var mean = sum / smoothArray.length;
        return mean;
    };

    this.getSmoothArray = function() {
        return smoothArray;
    }
}

var myHeading = document.querySelector('h1');

a = new DataAnalyzer(250);
myHeading.textContent = 'Hello world!';
a.addSample(1, new Date(2016,4,28,9,55,0,0));
a.addSample(1, new Date(2016,4,28,9,55,0,250));
a.addSample(1, new Date(2016,4,28,9,55,0,500));
a.addSample(4, new Date(2016,4,28,9,55,0,750));
a.addSample(4, new Date(2016,4,28,9,55,2,000));
a.addSample(1, new Date(2016,4,28,9,55,2,250));
samples = a.getSamples();
b = a.generateTalkIntervals();
console.log();
console.log("Done");
