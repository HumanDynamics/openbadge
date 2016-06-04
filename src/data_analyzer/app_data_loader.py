import json
import pandas as pd


class AppDataLoader(object):
    def __init__(self, filename):
        self.loadAppData(filename)

    def loadAppData(self,input_file_name):
        with open(input_file_name, 'r') as input_file:
            raw_data = input_file.readlines()  # This is a list of strings
            self.jasonMeetingMetadata = json.loads(raw_data[0])  # Convert the header string into a json object
            self.jasonChunks = map(json.loads, raw_data[1:])  # Convert the raw sample data into a json object

    def jasonChunksToSamples(self):
        sample_data = []

        for j in range(len(self.jasonChunks)):
            batch = {}
            batch.update(self.jasonChunks[j])  # Create a deep copy of the jth batch of samples
            samples = batch.pop('samples')
            reference_timestamp = batch.pop('timestamp') * 1000 + batch.pop(
                'timestamp_ms')  # reference timestamp in milliseconds
            sampleDelay = batch.pop('sampleDelay')
            numSamples = batch.pop('numSamples')
            if (numSamples != len(samples)):
                print("Error: number of samples doesn't match actaul array. Chunk# ", j)
                continue
            for i in range(numSamples):
                sample = {}
                sample.update(batch)
                sample['signal'] = samples[i]

                sample['timestamp'] = reference_timestamp + i * sampleDelay
                sample_data.append(sample)

        df_sample_data = pd.DataFrame(sample_data)
        df_sample_data['datetime'] = pd.to_datetime(df_sample_data['timestamp'], unit='ms')
        del df_sample_data['timestamp']

        df_sample_data.sort_values('datetime')
        # Optional: Add the meeting metadata to the dataframe
        # df_sample_data.metadata = meeting_metadata

        df_sample_data.set_index(pd.DatetimeIndex(df_sample_data['datetime']), inplace=True)
        df_sample_data.index.name = 'datetime'
        del df_sample_data['datetime']
        return df_sample_data

'''
    if(to_csv):
        output_file_name = input_file_name.split(".")[0] + ".csv"
        print "DataFrame written to "+output_file_name
        df_sample_data.to_csv(output_file_name)
        return None
    else:
        return df_sample_data
'''

if __name__ == "__main__":
    input_file_name = "app_data/879EO_1465008434236.txt.head"
    appData = AppDataLoader(input_file_name)
    print appData.jasonMeetingMetadata
    print appData.jasonChunks[0]
    df = appData.jasonChunksToSamples()
    print df.head()
