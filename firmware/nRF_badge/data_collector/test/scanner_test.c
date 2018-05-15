#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#include <scanner.h>
#include <scanner_test.h>


void make_test(){

    int beacons_n = rand() % 10;
    int badges_n = rand() % 50;
    
    scan.to = 0;
    scan.timestamp = 0;
    scan.num = beacons_n + badges_n;
    scan.numbeacons = beacons_n;
    
    unsigned short _IDs[scan.num];
    signed short _rssiSums[scan.num];
    signed char _counts[scan.num];
    
    for(int i = 0; i < scan.num; i++){
        _IDs[i] = 0;
        _counts[i] = 0;
        _rssiSums[i] = 0;
    }
       
    for(int i = 0; i < beacons_n; i++){
        int r = rand() % scan.num;
        while(_IDs[r] != 0){ 
            r = rand() % scan.num;
        }
  
        _IDs[r] = rand() % 997 + BEACON_ID_THRESHOLD;  // 997 is prime near 1000
    }
    
    
    for(int i = 0; i < scan.num; i++){
        if(_IDs[i] == 0){
            _IDs[i] = rand() % 997 + 100;
        }
        _counts[i] = rand() % 9 + 1;
        _rssiSums[i] = RESTORE_SAMPLE(-50 - (rand() % 30), _counts[i]);
    }
    
    memcpy((short *)scan.IDs, (short *)_IDs, 2*scan.num);
    memcpy((short *)scan.rssiSums, (short *)_rssiSums, 2*scan.num);
    memcpy((short *)scan.counts, (short *)_counts, 2*scan.num);
}

bool check_scan(void){
    bool good = true;
    int prior = (BEACON_PRIORITY < scan.numbeacons) ? BEACON_PRIORITY : scan.numbeacons;
    
    for(int i = 0; i < scan.num-1; i++){
        if(i < prior){
            if(scan.IDs[i] < BEACON_ID_THRESHOLD || (i < prior-1 && PROCESS_SAMPLE(scan.rssiSums[i], scan.counts[i]) < PROCESS_SAMPLE(scan.rssiSums[i+1], scan.counts[i+1]))){
                good = false;
                break;
            }
        }else{
            if(PROCESS_SAMPLE(scan.rssiSums[i], scan.counts[i]) < PROCESS_SAMPLE(scan.rssiSums[i+1], scan.counts[i+1])){
                good = false;
                break;
            }
        }
    }
    
    return good;
}


TEST_FILE scanner_tf(){
    TEST_FILE tf;
    char name[64] = "scanner_test.c";
    tf.name = name;
    tf.gen = make_test;
    tf.run = sortScanByRSSIDescending;
    tf.check = check_scan;
    return tf;
}

