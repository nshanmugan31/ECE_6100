#ifndef DRAM_H
#define DRAM_H

#include <stdint.h>
#include "types.h"

/////////////////////////////////////////////////////////////////////////////////////////////
// Define the Data Structures here with correct field (Look at Appendix B for more details)
/////////////////////////////////////////////////////////////////////////////////////////////
typedef struct DRAM DRAM;
typedef struct Rowbuf_Entry Rowbuf_Entry;

/*
“Row Buffer” Entry structure (Rowbuf_Entry) can have following entries:
●Valid
●Row ID (If the entry is valid, which row)
*/

struct Rowbuf_Entry{
    bool valid;
    uint64_t row_id;
};

/*
“DRAM” structure can have the following fields:
●Array of Rowbuf Entry (Maximum could be 256)
*/

struct DRAM{
    Rowbuf_Entry array[256];
    uint64_t stat_read_access;
    uint64_t stat_write_access;
    uint64_t stat_read_delay;
    uint64_t stat_write_delay;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Mandatory variables required for generating the desired final reports as necessary
// Used by dram_print_stats()
/////////////////////////////////////////////////////////////////////////////////////////////

// stat_read_access: Number of read (lookup accesses do not count as READ accesses) 
//                   accesses made to the DRAM
// stat_write_access: Number of write accesses made to the DRAM
// stat_read_delay: keeps track of cumulative DRAM read latency for subsequent incoming 
//                  READ requests to DRAM (only the latency spent at DRAM module)
// stat_write_delay: keeps track of cumulative DRAM write latency for subsequent incoming 
//                   WRITE requests to DRAM (only the latency paid at DRAM module)


/////////////////////////////////////////////////////////////////////////////////////////////
// Following functions might be helpful
/////////////////////////////////////////////////////////////////////////////////////////////

DRAM* dram_new();
void dram_print_stats(DRAM* dram);
uint64_t dram_access(DRAM* dram, Addr lineaddr, bool is_dram_write);
uint64_t dram_access_mode_CDE(DRAM* dram, Addr lineaddr, bool is_dram_write);


#endif // DRAM_H
