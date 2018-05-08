#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define NUM_TESTS 10
#define BEACON_PRIORITY 4
#define BEACON_ID_THRESHOLD 16000
#define MAX_AGGR true
#define MAX_SCAN_RESULTS 300

#if MAX_AGGR
	#define AGGR_SAMPLE(sample, datum) (datum > sample ? datum: sample)
	#define PROCESS_SAMPLE(aggregated, count) aggregated
	#define RESTORE_SAMPLE(processed, count) processed
#else
	#define AGGR_SAMPLE(sample, datum) (datum+sample)
	#define PROCESS_SAMPLE(aggregated, count) (aggregated/count)
	#define RESTORE_SAMPLE(processed, count) (processed*count) 
#endif

typedef struct
{
    unsigned short ID;
    signed char rssi;
    signed char count;
} seenDevice_t;

struct
{
    int to;
    volatile int timestamp;
    volatile int num, numbeacons;
    volatile unsigned short IDs[MAX_SCAN_RESULTS];
    volatile signed short rssiSums[MAX_SCAN_RESULTS];
    volatile signed char counts[MAX_SCAN_RESULTS];
} scan;

static int compareSeenDeviceByRSSI(const void * a, const void * b);
static int compareSeenDeviceByID(const void * a, const void * b);
static void sortScanByRSSIDescending(void);
void print_scan(void);
void make_test(void);
int check_scan(int);

int seed;
void main(){
    
    seed = time(NULL);
    int passed;
    
    for(int test = 0; test < NUM_TESTS; test++){
        srand(seed);
        
        make_test();
        sortScanByRSSIDescending();
        passed += check_scan(test);
        
        seed = rand();
    }
    printf("\nGOOD: %d  FAIL: %d\n\n", passed, NUM_TESTS-passed);
    
}

static void sortScanByRSSIDescending(void) {
    seenDevice_t seenDevices[scan.num];

    // Convert scan into an array of structs for sorting 
    for (int i = 0; i < scan.num; i++) {
        seenDevices[i].ID = scan.IDs[i];
        seenDevices[i].rssi = PROCESS_SAMPLE(scan.rssiSums[i], scan.counts[i]);
        seenDevices[i].count = scan.counts[i];
    }
    
    int prioritized = (BEACON_PRIORITY < scan.numbeacons) ? BEACON_PRIORITY : scan.numbeacons;
    qsort(seenDevices, (size_t)scan.num, sizeof(seenDevice_t), compareSeenDeviceByID);
    qsort(seenDevices, (size_t)scan.numbeacons, sizeof(seenDevice_t), compareSeenDeviceByRSSI);
    qsort(seenDevices+prioritized, (size_t)(scan.num-prioritized), sizeof(seenDevice_t), compareSeenDeviceByRSSI);
    
    for (int i = 0; i < scan.num; i++) {
        scan.IDs[i] = seenDevices[i].ID;
        scan.rssiSums[i] = RESTORE_SAMPLE(seenDevices[i].rssi, seenDevices[i].count);
        scan.counts[i] = seenDevices[i].count;
    }
}

static int compareSeenDeviceByRSSI(const void * a, const void * b) {
    seenDevice_t * seenDeviceA = (seenDevice_t *) a;
    seenDevice_t * seenDeviceB = (seenDevice_t *) b;

    if (seenDeviceA->rssi > seenDeviceB->rssi) {
        return -1; // We want device A before device B in our list.
    } else if (seenDeviceA->rssi == seenDeviceB->rssi) {
        return 0; // We don't care whether deviceA or deviceB comes first.
    } else if (seenDeviceA->rssi < seenDeviceB->rssi) {
        return 1; // We want device A to come after device B in our list.
    }

    // We should never get here?
    // APP_ERROR_CHECK_BOOL(false);

    return -1;
}


static int compareSeenDeviceByID(const void * a, const void * b) {
    seenDevice_t * seenDeviceA = (seenDevice_t *) a;
    seenDevice_t * seenDeviceB = (seenDevice_t *) b;

    if (seenDeviceA->ID > seenDeviceB->ID) {
        return -1; // We want device A before device B in our list.
    } else if (seenDeviceA->ID == seenDeviceB->ID) {
        return 0; // We don't care whether deviceA or deviceB comes first.
    } else if (seenDeviceA->ID < seenDeviceB->ID) {
        return 1; // We want device A to come after device B in our list.
    }

    // We should never get here?
    // APP_ERROR_CHECK_BOOL(false);

    return -1;
}

void print_scan(){
    printf("\nIDs:\t{");
    for(int i = 0; i < scan.num-1; i++){
        printf("%d,\t", scan.IDs[i]);    
    }
    printf("%d}\n\nrssiSums:{", scan.IDs[scan.num-1]);
    
    for(int i = 0; i < scan.num-1; i++){
        printf("%d,\t", scan.rssiSums[i]);    
    }
    printf("%d}", scan.rssiSums[scan.num-1]);
    printf("\n");
}

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
}

int check_scan(int test){
    bool good = true;
    int prior = (BEACON_PRIORITY < scan.numbeacons) ? BEACON_PRIORITY : scan.numbeacons;
    
    print_scan();
    
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
    
    
    if(good){
        printf("Test %3d: GOOD seed: %d\t nbeac: %d  nbadg: %2d  prior: %d\n", test, seed, scan.numbeacons, scan.num-scan.numbeacons, prior);
    }else{
        printf("Test %3d: FAIL seed: %d\t nbeac: %d  nbadg: %2d  prior: %d\n", test, seed, scan.numbeacons, scan.num-scan.numbeacons, prior);
    }
    
    return good;
}

