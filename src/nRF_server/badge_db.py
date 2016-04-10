from __future__ import print_function
import sqlite3
import datetime

class badgeDB():
    db_file_name = "data.db"
    t_samples = "samples"
    t_chunks = "chunks"
    conn = None
    
    def __init__(self):
        self.conn = sqlite3.connect(self.db_file_name, detect_types=sqlite3.PARSE_DECLTYPES)
        print("Openning db")
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
        print("Commiting")
        self.conn.commit()
        print("Closing db")
        self.conn.close()

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
            return datetime.datetime.strptime(ret[0],'%Y-%m-%d %H:%M:%S.%f')

if __name__ == "__main__":
    with badgeDB() as db:
        db.insertSample('AAA',datetime.datetime.now(),3)
        db.insertChunk('AAA',datetime.datetime.now(),2.888)
        a = db.getLastChunkDate('AAB')
        print(a)
        a = db.getLastChunkDate('AAA')
        print (a)