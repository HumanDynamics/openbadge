//external libraries
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

//build source
#include <scanner.h>

//test source
#include <scanner_test.h>

TEST_FILE test_files[NUM_TESTS] = {{scanner_tf}};

int main(){
    
    int seed = time(NULL);
    int testpass = 0, casepass, test;
    TEST_FILE file;    
    
    for(test = 0; test < NUM_TESTS; test++){
        srand(seed);
        
        file = test_files[test];
        printf("Test %2d: %s ", test, file.name);      
        
        casepass = 0;
        for(int iter = 0; iter < NUM_ITERS; iter++){
            generate_case(file.gen);
            run_case(file.run);
            
            if(check_case(file.check)){
                if(iter % 100 == 0){
                    printf(" .");
                }
                casepass++;
            }else{
                break;
            }
            
            seed = rand();
        }
        
        if(casepass == NUM_ITERS){
            testpass++;
            printf("OK\n");
        }else{
            printf("*ERROR* seed: %d\n", seed);
        }
    }
    
    if(testpass == NUM_TESTS) 
        return 0;
    return -1;
}


void generate_case(CASE_GENERATOR cg){
    cg();
}

void run_case(CASE_RUNNER cr){
    cr();
}

bool check_case(CASE_CHECKER cc){
    return cc();
}

