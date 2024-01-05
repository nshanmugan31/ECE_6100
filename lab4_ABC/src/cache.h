#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>
#include "types.h"


/////////////////////////////////////////////////////////////////////////////////////////////
// Define the necessary data structures here (Look at Appendix A for more details)
/////////////////////////////////////////////////////////////////////////////////////////////

//Define the variables
typedef struct Cache_Line Cache_Line;
typedef struct Cache_Set Cache_Set;
typedef struct Cache Cache;

typedef struct Utility_Monitor_Struct Utility_Monitor_Struct;
typedef struct Temporary_Cache Temporary_Cache;


struct Utility_Monitor_Struct
{
  uint64_t number_ways;
  uint64_t replacement_policy;
  uint64_t number_sets;

  Cache *Auxiliary_Tag_Directory;
  uint64_t *counter;
};

struct Temporary_Cache{
    uint64_t number_ways; //number of ways 
    //uint64_t replacement_policy //Needs to follow the variable name 
    uint64_t replacement_policy; //Replacement policy
    uint64_t number_sets; //number of sets  
};



/*
“Cache Line” structure (Cache_Line), will have the following fields:
Valid: denotes if the cache line is indeed present in the Cache
●Dirty: denotes if the latest data value is present only in the local Cache 
●Tag: denotes the conventional higher-order address bits beyond Index 
●Core ID: needed to identify the core to which a cache line (way) is
assigned to in a multicore scenario (required for Part D, E, F)
●Insertion Time: to keep track of when each line was inserted, which helps
with the FIFO replacement policy
*/
struct Cache_Line {
    bool valid;  //checks if cache line is present in the cache
    bool dirty;  //checks if the latest data value is present only in the local Cache
    Addr tag;    // denotes the conventional higher-order address bits beyond Index
    //This will throw an error unless Addr is included
    uint32_t core_id; //needed to identify the core to which a cache line (way) is assigned to in a multicore scenario
    uint32_t insertion_time; //to keep track of when each line was inserted
};

/*
“Cache Set” structure (Cache_Set), will have:
●Cache_Line Struct (replicated “# of Ways” times, as in an array/list)
*/

struct Cache_Set {
    //Takes the number of ways as an array or list 
    //We know the number of ways is 16 
    //Need to use the cache line struct 
    Cache_Line cache_line[16];
};

/*
The overarching “Cache” structure should have:
●Cache_Set Struct (replicated “#Sets” times, as in a list/array)
●# of Ways
●Replacement Policy
●# of Sets
●Last evicted Line (Cache_Line type) to be passed on to next higher cache
hierarchy for an install if necessary

stat_read_access: Number of read (lookup accesses do not count as READ accesses)
accesses made to the cache
●stat_write_access: Number of write accesses made to the cache
●stat_read_miss: Number of READ requests that lead to a MISS at the respective cache
●stat_write_miss: Number of WRITE requests that lead to a MISS at the respective
cache
●stat_dirty_evicts: Count of requests to evict DIRTY lines
*/

struct Cache{
    Cache_Set *cache_sets; //Cache_set Struct
    uint64_t number_ways; //number of ways 
    //uint64_t replacement_policy //Needs to follow the variable name 
    uint64_t replacement_policy; //Replacement policy
    uint64_t number_sets; //number of sets
    Cache_Line last_evicted_line; //Last evicted Line (Cache_Line type) to be passed on to next higher cache hierarchy for an install if necessary

    uint64_t stat_read_access; //Number of read (lookup accesses do not count as READ accesses accesses made to the cache
    uint64_t stat_write_access; //Number of write accesses made to the cache
    uint64_t stat_read_miss; //Number of READ requests that lead to a MISS at the respective cache
    uint64_t stat_write_miss; //Number of WRITE requests that lead to a MISS at the respective cache
    uint64_t stat_dirty_evicts; //Count of requests to evict DIRTY lines  
    Utility_Monitor_Struct *utility_monitor_struct[2];
};
/////////////////////////////////////////////////////////////////////////////////////////////
// Mandatory variables required for generating the desired final reports as necessary
// Used by cache_print_stats()
/////////////////////////////////////////////////////////////////////////////////////////////

// stat_read_access: Number of read (lookup accesses do not count as READ accesses)
//                   accesses made to the cache
// stat_write_access: Number of write accesses made to the cache
// stat_read_miss: Number of READ requests that lead to a MISS at the respective cache
// stat_write_miss: Number of WRITE requests that lead to a MISS at the respective cache
// stat_dirty_evicts: Count of requests to evict DIRTY lines

/////////////////////////////////////////////////////////////////////////////////////////////
// Functions to be implemented
/////////////////////////////////////////////////////////////////////////////////////////////

Cache* cache_new(uint64_t size, uint64_t assocs, uint64_t linesize, uint64_t repl_policy);
bool cache_access(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
void cache_install(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id);
uint32_t cache_find_victim(Cache* c, uint32_t set_index, uint32_t core_id);

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

void cache_print_stats(Cache* c, char* header);

Utility_Monitor_Struct* utility_monitor_struct_new(uint64_t size, uint64_t assocs, uint64_t linesize, uint64_t repl_policy);
Temporary_Cache* temporary_cache_new(uint64_t size, uint64_t assocs, uint64_t linesize, uint64_t repl_policy);

#endif // CACHE_H
