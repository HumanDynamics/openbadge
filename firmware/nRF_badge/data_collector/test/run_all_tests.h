#include <stdbool.h>

#define NUM_TESTS 1 
#define NUM_ITERS 1000

typedef void (*CASE_GENERATOR)();
typedef void (*CASE_RUNNER)();
typedef bool (*CASE_CHECKER)();

typedef struct {
    char *name;
    CASE_GENERATOR gen;
    CASE_RUNNER run;
    CASE_CHECKER check;
} TEST_FILE;

void generate_case(CASE_GENERATOR cg);
void run_case(CASE_RUNNER cr);
bool check_case(CASE_CHECKER cc);

