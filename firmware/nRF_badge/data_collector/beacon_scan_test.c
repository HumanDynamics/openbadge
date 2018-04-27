#include <stdio.h>
#include <stdlib.h>

void printarray(int arr[], int dim);
static int compare(const void *a, const void *b);
void interlace_upto_n(int * dest, int dest_n, int * source1, int s1_n, int * source2, int s2_n);
void sort_k_beacon_priority(int k, int *store, int store_n, int *beacons, int beacons_n, int *badges, int badges_n);


void main(){
    int beacons[22] = {41,14,73, 27,70,13,345,68,31,226,53,6,68,32,44,30, -43, -8, 23, 29, 40, 11};
    int badges[26] = {34, 7, 3, -50, 93, 99, 24, 232, 54, 253, 65, 7,95, 43, 10, 41, 1234, 1, 2, 4, 200, 300, -1, 21345, 29, 0};
    int store[32];
    
    sort_k_beacon_priority(3, store, 32, beacons, 22, badges, 26);
	
    printarray(beacons, 22);
    printarray(badges, 26);
    printf("\n");
    printarray(store, 32);
    printf("\n");
}

void sort_k_beacon_priority(int k, int *store, int store_n, int *beacons, int beacons_n, int *badges, int badges_n){
    
    // initially sort both the arrays and beacons
    qsort(beacons, beacons_n, sizeof(int), compare);    
    qsort(badges, badges_n, sizeof(int), compare);
    
    for(int i = 0; i < k; i++){ // store the top k beacons
        store[i] = beacons[i];
    }
    
    // interlace the remaining dest_n-k beacons and badges by the compare function
    interlace_upto_n(&store[k], store_n-k, &beacons[k], beacons_n-k, badges, badges_n);
}

void interlace_upto_n(int * dest, int dest_n,
                    int * source1, int s1_n, 
                    int * source2, int s2_n){

    // Use a two-finger algorithm to interlace the source arrays into the destination by the function compare    
    int dest_i;
    int s1_i = 0;
    int s2_i = 0;
    int comp;
    
    // Two finger part:
    for(dest_i = 0; dest_i < dest_n; dest_i++){
        comp = compare(&source1[s1_i], &source2[s2_i]);
        
        if(comp >= 0){
            dest[dest_i] = source2[s2_i++];
            if(s2_i == s2_n) break;
    
        }else{
            dest[dest_i] = source1[s1_i++];
            if(s1_i == s1_n) break;
        }
    }
    
    // One finger reached limit, fill the rest as much as possible with the other array
    if(s1_i == s1_n){
        int lim = s2_n-s2_i < dest_n-dest_i ? s2_n-s2_i : dest_n-dest_i;
        
        for(int i = 0; i < lim; i++){
            dest[dest_i++] = source2[s2_i++];
        }
    
    }else if(s2_i == s2_n){
        int lim = s1_n-s1_i < dest_n-dest_i ? s1_n-s1_i : dest_n-dest_i;
        
        for(int i = 0; i < lim; i++){
            dest[dest_i++] = source1[s1_i++];
        }
    }
}

static int compare(const void *a, const void *b){
    int aint = *(int *)a;
    int bint = *(int *)b;

    if(aint>bint) return -1;
    if(aint<bint) return +1;
    return 0;
}

void printarray(int arr[], int dim){
    printf("\n{");
    for(int i = 0; i < dim-1; i++){
        printf("%d, ", arr[i]);    
    }
    printf("%d}", arr[dim-1]);
}

