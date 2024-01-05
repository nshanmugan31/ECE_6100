#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>

#include "memsys.h"

#define PAGE_SIZE 4096

//---- Cache Latencies  ------

#define DCACHE_HIT_LATENCY   1
#define ICACHE_HIT_LATENCY   1
#define L2CACHE_HIT_LATENCY  10

extern MODE   SIM_MODE;
extern uint64_t  CACHE_LINESIZE;
extern uint64_t  REPL_POLICY;

extern uint64_t  DCACHE_SIZE;
extern uint64_t  DCACHE_ASSOC;
extern uint64_t  ICACHE_SIZE;
extern uint64_t  ICACHE_ASSOC;
extern uint64_t  L2CACHE_SIZE;
extern uint64_t  L2CACHE_ASSOC;
extern uint64_t  L2CACHE_REPL;
extern uint64_t  NUM_CORES;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


Memsys* memsys_new(void){
	Memsys* sys = (Memsys*)calloc(1, sizeof (Memsys));

	switch(SIM_MODE) {
		case SIM_MODE_A:
			sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			break;  

		case SIM_MODE_B:
		case SIM_MODE_C:
			sys->dcache = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			sys->icache = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			sys->dram = dram_new();
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			for (int i=0; i<NUM_CORES; i++) {
				sys->dcache_coreid[i] = cache_new(DCACHE_SIZE, DCACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
				sys->icache_coreid[i] = cache_new(ICACHE_SIZE, ICACHE_ASSOC, CACHE_LINESIZE, REPL_POLICY);
			}
			sys->l2cache = cache_new(L2CACHE_SIZE, L2CACHE_ASSOC, CACHE_LINESIZE, L2CACHE_REPL);
			sys->dram = dram_new();
			break;
		default:
			break;
	}

	return sys;
}


////////////////////////////////////////////////////////////////////
// Return the latency of a memory operation
////////////////////////////////////////////////////////////////////

uint64_t memsys_access(Memsys* sys, Addr addr, Access_Type type, uint32_t core_id){
	uint32_t delay = 0;

	// all cache transactions happen at line granularity, so get lineaddr
	Addr lineaddr = addr / CACHE_LINESIZE;

	switch (SIM_MODE) {
		case SIM_MODE_A:
			delay = memsys_access_modeA(sys,lineaddr,type,core_id);
			break;

		case SIM_MODE_B:
		case SIM_MODE_C:
			delay = memsys_access_modeBC(sys,lineaddr,type,core_id);
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			delay = memsys_access_modeDE(sys,lineaddr,type,core_id);
			break;
		default:
			break;
	}

	//update the stats

	switch (type) {
		case ACCESS_TYPE_IFETCH: 
			sys->stat_ifetch_access++;
			sys->stat_ifetch_delay += delay;
			break;
		case ACCESS_TYPE_LOAD:
			sys->stat_load_access++;
			sys->stat_load_delay += delay;
			break;
		case ACCESS_TYPE_STORE:
			sys->stat_store_access++;
			sys->stat_store_delay += delay;
			break;
		default:
			break;
	}

	return delay;
}



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void memsys_print_stats(Memsys* sys){
	char header[256];
	sprintf(header, "MEMSYS");

	double ifetch_delay_avg=0, load_delay_avg=0, store_delay_avg=0;

	if (sys->stat_ifetch_access) {
		ifetch_delay_avg = (double)(sys->stat_ifetch_delay) / (double)(sys->stat_ifetch_access);
	}

	if (sys->stat_load_access) {
		load_delay_avg = (double)(sys->stat_load_delay) / (double)(sys->stat_load_access);
	}

	if (sys->stat_store_access) {
		store_delay_avg = (double)(sys->stat_store_delay) / (double)(sys->stat_store_access);
	}


	printf("\n");
	printf("\n%s_IFETCH_ACCESS  \t\t : %10llu",  header, sys->stat_ifetch_access);
	printf("\n%s_LOAD_ACCESS    \t\t : %10llu",  header, sys->stat_load_access);
	printf("\n%s_STORE_ACCESS   \t\t : %10llu",  header, sys->stat_store_access);
	printf("\n%s_IFETCH_AVGDELAY\t\t : %10.3f",  header, ifetch_delay_avg);
	printf("\n%s_LOAD_AVGDELAY  \t\t : %10.3f",  header, load_delay_avg);
	printf("\n%s_STORE_AVGDELAY \t\t : %10.3f",  header, store_delay_avg);
	printf("\n");

	switch (SIM_MODE) {
		case SIM_MODE_A:
			sprintf(header, "DCACHE");
			cache_print_stats(sys->dcache, header);
			break;
		case SIM_MODE_B:
		case SIM_MODE_C:
			sprintf(header, "ICACHE");
			cache_print_stats(sys->icache, header);
			sprintf(header, "DCACHE");
			cache_print_stats(sys->dcache, header);
			sprintf(header, "L2CACHE");
			cache_print_stats(sys->l2cache, header);
			dram_print_stats(sys->dram);
			break;

		case SIM_MODE_D:
		case SIM_MODE_E:
			assert(NUM_CORES==2); //Hardcoded
			sprintf(header, "ICACHE_0");
			cache_print_stats(sys->icache_coreid[0], header);
			sprintf(header, "DCACHE_0");
			cache_print_stats(sys->dcache_coreid[0], header);
			sprintf(header, "ICACHE_1");
			cache_print_stats(sys->icache_coreid[1], header);
			sprintf(header, "DCACHE_1");
			cache_print_stats(sys->dcache_coreid[1], header);
			sprintf(header, "L2CACHE");
			cache_print_stats(sys->l2cache, header);
			dram_print_stats(sys->dram);
			break;
		default:
			break;
	}
}


////////////////////////////////////////////////////////////////////
// Used by Part A to access the L1 cache
////////////////////////////////////////////////////////////////////

uint64_t memsys_access_modeA(Memsys* sys, Addr lineaddr, Access_Type type, uint32_t core_id){
  
	// IFETCH accesses go to icache, which we don't have in part A
	bool needs_dcache_access = !(type == ACCESS_TYPE_IFETCH);

	// Stores write to the caches
	bool is_write = (type == ACCESS_TYPE_STORE);

	if (needs_dcache_access) {
		// Miss
		if (!cache_access(sys->dcache,lineaddr,is_write,core_id)) {
			// Install the new line in L1
			cache_install(sys->dcache,lineaddr,is_write,core_id);
		}
	}

	// Timing is not simulated in Part A
	return 0;
}

////////////////////////////////////////////////////////////////////
// Used by Parts B,C to access the L1 icache + L1 dcache
// Returns the access latency
////////////////////////////////////////////////////////////////////

uint64_t memsys_access_modeBC(Memsys* sys, Addr lineaddr, Access_Type type, uint32_t core_id){
	uint64_t delay;
	//Initialize delay to zero
	delay = 0;

	// Perform the icache/dcache access
	bool needs_icache_access = (type == ACCESS_TYPE_IFETCH);
	bool needs_dcache_access = !(type == ACCESS_TYPE_IFETCH);

	//Check Instruction Cache
	if(needs_icache_access)
	{
		//if instruction cache hit
		//Instruction Cache hit latency is 1
		//store is not used 
		bool is_write = false;
		if(cache_access(sys->icache, lineaddr, is_write, core_id))
		{
			delay = ICACHE_HIT_LATENCY;
		}
		//Instruction cache is a miss
		else
		{
			//Delay for accessing l2 
			uint64_t l2_access_delay = memsys_L2_access(sys, lineaddr, is_write, core_id);
			//Delay is the instruction cache latency plus the latency of l2 access
			delay = ICACHE_HIT_LATENCY + l2_access_delay; 

			//Install cache instruction 
			cache_install(sys->icache, lineaddr, is_write, core_id);
		}
	
	}

	if(needs_dcache_access)
	{
		bool is_write;
		if(type == ACCESS_TYPE_STORE)
		{
			is_write = true;
		}
		if(type == ACCESS_TYPE_LOAD)
		{
			is_write = false;
		}
		//Data cache hit
		if(cache_access(sys->dcache, lineaddr, is_write, core_id))
		{
			delay = DCACHE_HIT_LATENCY;
		}
		// On dcache miss, access the L2 + install the new line + if needed, perform writeback
		else
		{
			bool is_writeback = false;
			//Delay for accessing l2 
			uint64_t l2_access_delay = memsys_L2_access(sys, lineaddr, is_writeback, core_id);
			//Delay is the data cache latency plus the latency of l2 access
			delay = DCACHE_HIT_LATENCY + l2_access_delay; 

			//Install data instruction 
			cache_install(sys->dcache, lineaddr, is_write, core_id);

			//if the last cache line has been evicted and it is dirty, writeback
			if(sys->dcache->last_evicted_line.dirty == true)
			{
				//uint64_t cache_tag = lineaddr / c->number_sets;
				//uint64_t cache_index = (lineaddr % c->number_sets);

				//Line address is tag and index with out the offset 
				bool is_writeback = true; 
				uint64_t last_evicted_line_address;
				uint64_t last_evicted_line_tag = sys->dcache->last_evicted_line.tag;
				uint64_t last_evicted_line_number_sets = sys->dcache->number_sets;
				uint64_t last_evicted_line_index = lineaddr % sys->dcache->number_sets;

				uint64_t last_evicted_line_tag_bits  = last_evicted_line_tag * last_evicted_line_number_sets;

				last_evicted_line_address = last_evicted_line_tag_bits + last_evicted_line_index; 

				//Write the last_evicted_line to L2
				memsys_L2_access(sys, last_evicted_line_address , is_writeback, core_id);
			}
		}
	}
	

	return delay;
}

////////////////////////////////////////////////////////////////////
// Used by Parts B,C to access the L2
// Returns the access latency
////////////////////////////////////////////////////////////////////

uint64_t memsys_L2_access(Memsys* sys, Addr lineaddr, bool is_writeback, uint32_t core_id){ 
	uint64_t delay = L2CACHE_HIT_LATENCY;
	//L2 cache is a hit
	if(cache_access(sys->l2cache, lineaddr, is_writeback, core_id))
	{
		delay = L2CACHE_HIT_LATENCY;
	}
	//L2 cache is a miss
	else
	{
		if(is_writeback == false)
		{
			bool is_dram_write = false;
			//Delay for accessing l2 
			uint64_t dram_access_delay = dram_access(sys->dram, lineaddr, is_dram_write);
			//Delay is the dram latency plus the latency of l2 hit
			delay = L2CACHE_HIT_LATENCY + dram_access_delay; 

			//Install into L2 cache
			cache_install(sys->l2cache, lineaddr, is_writeback, core_id);
		}
		else
		{
			//Just want to install the line
			delay = L2CACHE_HIT_LATENCY; 
			//Install into L2 cache
			cache_install(sys->l2cache, lineaddr, is_writeback, core_id);
		}

		//if the last cache line has been evicted and it is dirty 
		if(sys->l2cache->last_evicted_line.dirty == true)
		{
			bool is_dram_write = true;
			//uint64_t cache_tag = lineaddr / c->number_sets;
			//uint64_t cache_index = (lineaddr % c->number_sets);

			//Line address is tag and index with out the offset
			//figure out the last_evicted_line_address 
			uint64_t last_evicted_line_address;
			uint64_t last_evicted_line_tag = sys->l2cache->last_evicted_line.tag;
			uint64_t last_evicted_line_number_sets = sys->l2cache->number_sets;
			uint64_t last_evicted_line_index = lineaddr % sys->l2cache->number_sets;

			uint64_t last_evicted_line_tag_bits  = last_evicted_line_tag * last_evicted_line_number_sets;

			last_evicted_line_address = last_evicted_line_tag_bits + last_evicted_line_index; 

			//Write the last_evicted_line to dram
			dram_access(sys->dram, last_evicted_line_address, is_dram_write);
		}
	}
	return delay;
}

/////////////////////////////////////////////////////////////////////
// Converts virtual page number (VPN) to physical frame number (PFN)
// Note, you will need additional operations to obtain
// VPN from lineaddr and to get physical lineaddr using PFN.
/////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// ------------- DO NOT CHANGE THE CODE OF THIS FUNCTION ----------
////////////////////////////////////////////////////////////////////

uint64_t memsys_convert_vpn_to_pfn(Memsys *sys, uint64_t vpn, uint32_t core_id){
	uint64_t tail = vpn & 0x000fffff;
	uint64_t head = vpn >> 20;
	uint64_t pfn  = tail + (core_id << 21) + (head << 21); 
	assert(NUM_CORES==2);
	return pfn;
}


/////////////////////////////////////////////////////////////////////
// Used by Parts D,E to access per-core L1 icache + L1 dcache
/////////////////////////////////////////////////////////////////////

uint64_t memsys_access_modeDE(Memsys* sys, Addr v_lineaddr, Access_Type type, uint32_t core_id){
	//uint64_t delay;

	// First convert lineaddr from virtual (v) to physical (p) using the
	// function memsys_convert_vpn_to_pfn(). Page size is defined to be 4KB.
	// NOTE: VPN_to_PFN operates at page granularity and returns page addr

	//return delay;

	//Virtual Page Address is Virtual Page Number + Offset

	//uint64_t number_of_sets = c->number_sets;
	//uint64_t cache_index = (lineaddr % number_of_sets);
	//uint64_t cache_tag = lineaddr / number_of_sets;
	//Lineaddr = tag + index
	//Lineaddr = addr /CACHE_LINESIZE
	// addr = CACHE_LINESIZE * Lineaddr
	//Line address mode page size

	uint64_t cache_linesize = CACHE_LINESIZE;
	uint64_t page_size = PAGE_SIZE;
	uint64_t icache_hit_latency = ICACHE_HIT_LATENCY;
	uint64_t dcache_hit_latency = DCACHE_HIT_LATENCY;
	//virtual_page_number = A/page_size
	uint64_t addr = cache_linesize * v_lineaddr;
	uint64_t virtual_page_number = addr / page_size;
	uint64_t physical_frame_number;
	physical_frame_number = memsys_convert_vpn_to_pfn(sys, virtual_page_number, core_id);
	//offset = A mod page_size
	uint64_t offset = addr % page_size;
	uint64_t physical_page_number_bits = physical_frame_number * page_size;
	uint64_t physical_address = physical_page_number_bits + offset;
	//Lineaddr = addr /CACHE_LINESIZE
	uint64_t physical_lineaddr = physical_address / cache_linesize;

	uint64_t delay;
	//Initialize delay to zero
	delay = 0;

	// Perform the icache/dcache access
	bool needs_icache_access = (type == ACCESS_TYPE_IFETCH);
	bool needs_dcache_access = !(type == ACCESS_TYPE_IFETCH);

	//Check Instruction Cache
	if(needs_icache_access)
	{
		//if instruction cache hit
		//Instruction Cache hit latency is 1
		//store is not used 
		bool is_write = false;
		if(cache_access(sys->icache_coreid[core_id], physical_lineaddr, is_write, core_id))
		{
			delay = icache_hit_latency;
		}
		//Instruction cache is a miss
		else
		{
			//Delay for accessing l2 
			uint64_t l2_access_delay = memsys_L2_access_multicore(sys, physical_lineaddr, is_write, core_id);
			//Delay is the instruction cache latency plus the latency of l2 access
			delay = icache_hit_latency + l2_access_delay; 

			//Install cache instruction 
			cache_install(sys->icache_coreid[core_id], physical_lineaddr, is_write, core_id);
		}
	
	}

	if(needs_dcache_access)
	{
		bool is_write;
		if(type == ACCESS_TYPE_STORE)
		{
			is_write = true;
		}
		if(type == ACCESS_TYPE_LOAD)
		{
			is_write = false;
		}
		//Data cache hit
		if(cache_access(sys->dcache_coreid[core_id], physical_lineaddr, is_write, core_id))
		{
			delay = dcache_hit_latency;
		}
		// On dcache miss, access the L2 + install the new line + if needed, perform writeback
		else
		{
			bool is_writeback = false;
			//Delay for accessing l2 
			uint64_t l2_access_delay = memsys_L2_access_multicore(sys, physical_lineaddr, is_writeback, core_id);
			//Delay is the data cache latency plus the latency of l2 access
			delay = dcache_hit_latency + l2_access_delay; 

			//Install data instruction 
			cache_install(sys->dcache_coreid[core_id], physical_lineaddr, is_write, core_id);

			//if the last cache line has been evicted and it is dirty, writeback
			if(sys->dcache_coreid[core_id]->last_evicted_line.dirty == true)
			{
				//uint64_t cache_tag = lineaddr / c->number_sets;
				//uint64_t cache_index = (lineaddr % c->number_sets);

				//Line address is tag and index with out the offset 
				bool is_writeback = true; 
				uint64_t last_evicted_line_address;
				uint64_t last_evicted_line_tag = sys->dcache_coreid[core_id]->last_evicted_line.tag;
				uint64_t last_evicted_line_number_sets = sys->dcache_coreid[core_id]->number_sets;
				uint64_t last_evicted_line_index = physical_lineaddr % sys->dcache_coreid[core_id]->number_sets;

				uint64_t last_evicted_line_tag_bits  = last_evicted_line_tag * last_evicted_line_number_sets;

				last_evicted_line_address = last_evicted_line_tag_bits + last_evicted_line_index; 

				//Write the last_evicted_line to L2
				memsys_L2_access_multicore(sys, last_evicted_line_address , is_writeback, core_id);
			}
		}
	}
	

	return delay;
}


/////////////////////////////////////////////////////////////////////
// Used by Parts D,E to access the L2
/////////////////////////////////////////////////////////////////////

uint64_t memsys_L2_access_multicore(Memsys* sys, Addr lineaddr, bool is_writeback, uint32_t core_id){
	uint64_t delay = L2CACHE_HIT_LATENCY;
	//L2 cache is a hit
	if(cache_access(sys->l2cache, lineaddr, is_writeback, core_id))
	{
		delay = L2CACHE_HIT_LATENCY;
	}
	//L2 cache is a miss
	else
	{
		if(is_writeback == false)
		{
			bool is_dram_write = false;
			//Delay for accessing l2 
			uint64_t dram_access_delay = dram_access(sys->dram, lineaddr, is_dram_write);
			//Delay is the dram latency plus the latency of l2 hit
			delay = L2CACHE_HIT_LATENCY + dram_access_delay; 

			//Install into L2 cache
			cache_install(sys->l2cache, lineaddr, is_writeback, core_id);
		}
		else
		{
			//Just want to install the line
			delay = L2CACHE_HIT_LATENCY; 
			//Install into L2 cache
			cache_install(sys->l2cache, lineaddr, is_writeback, core_id);
		}

		//if the last cache line has been evicted and it is dirty 
		if(sys->l2cache->last_evicted_line.dirty == true)
		{
			bool is_dram_write = true;
			//uint64_t cache_tag = lineaddr / c->number_sets;
			//uint64_t cache_index = (lineaddr % c->number_sets);

			//Line address is tag and index with out the offset
			//figure out the last_evicted_line_address 
			uint64_t last_evicted_line_address;
			uint64_t last_evicted_line_tag = sys->l2cache->last_evicted_line.tag;
			uint64_t last_evicted_line_number_sets = sys->l2cache->number_sets;
			uint64_t last_evicted_line_index = lineaddr % sys->l2cache->number_sets;

			uint64_t last_evicted_line_tag_bits  = last_evicted_line_tag * last_evicted_line_number_sets;

			last_evicted_line_address = last_evicted_line_tag_bits + last_evicted_line_index; 

			//Write the last_evicted_line to dram
			dram_access(sys->dram, last_evicted_line_address, is_dram_write);
		}
	}
	return delay;
}

