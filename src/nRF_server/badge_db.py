from __future__ import print_function
import sqlite3
import datetime

class badgeDB():
    db_file_name = "data.db"
    t_samples = "samples"
    t_chunks = "chunks"
    conn = None
    
    def __init__(self):
        self.conn = sqlite3.connect(self.db_file_name)
        #print("Openning db")
        # create table if needed
        sql = 'create table if not exists ' + self.t_samples + '(' + \
            'mac TEXT'+\
            ',sample_ts TIMESTAMP'+\
            ',sample_value INTEGER'+\
            ')'
        self.conn.execute(sql)

        sql = 'create table if not exists ' + self.t_chunks + '(' + \
            'mac TEXT'+\
            ',chunk_ts TIMESTAMP'+\
            ',chunk_voltage FLOAT'+\
            ')'

        self.conn.execute(sql)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        #print("Commiting")
        self.conn.commit()
        #print("Closing db")
        self.conn.close()

    def insertSamples(self,mac, chunk):
        samples = [{"ts": chunk.ts + datetime.timedelta(milliseconds=mul*chunk.sampleDelay), "value": int(i)} for mul, i in enumerate(chunk.samples)]
        for s in samples:
            self.insertSample(mac,s['ts'],s['value'])

    def insertSample(self, mac,ts,value):
        self.conn.execute('insert into '+self.t_samples+' values(?, ?, ?)', (mac, ts, value))

    def insertChunk(self, mac,ts, voltage):
        self.conn.execute('insert into '+self.t_chunks+' values(?, ? ,?)', (mac, ts, voltage))

    def getLastChunkDate(self, mac):
        c = self.conn.cursor()
        #c.execute('select max(chunk_ts) from chunks where mac = ?',(mac,))
        c.execute('select max(chunk_ts) from chunks where mac = ?',(mac,))
        ret = c.fetchone()
        if ret == None or ret[0] == None:
            return None
        else:
            # Where microseconds = 0 the date is returned with there is no fractional part
            datestr = ret[0]
            if datestr.find(".") >0 :
                return datetime.datetime.strptime(datestr,'%Y-%m-%d %H:%M:%S.%f')
            else:
                return datetime.datetime.strptime(datestr,'%Y-%m-%d %H:%M:%S')

if __name__ == "__main__":
    with badgeDB() as db:
        db.insertSample('AAA',datetime.datetime.now(),3)
        db.insertChunk('AAA',datetime.datetime.now(),2.888)
        a = db.getLastChunkDate('AAB')
        print(a)
        a = db.getLastChunkDate('AAA')
        print (a)

        # Do we handle cases with 0 microseconds well?
        n = datetime.datetime.now().replace(microsecond=0)
        print(n)
        db.insertChunk('BBB',n,2.888)
        a = db.getLastChunkDate('BBB')
        print (a)