#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "cache.h"
#include <array>
extern uint64_t cycle;
extern uint64_t SWP_CORE0_WAYS; 
bool VERBOSE = false;

/////////////////////////////////////////////////////////////////////////////////////
// ---------------------- DO NOT MODIFY THE PRINT STATS FUNCTION --------------------
/////////////////////////////////////////////////////////////////////////////////////


void cache_print_stats(Cache* c, char* header){
	double read_mr = 0;
	double write_mr = 0;

	if (c->stat_read_access) {
		read_mr = (double)(c->stat_read_miss) / (double)(c->stat_read_access);
	}

	if (c->stat_write_access) {
		write_mr = (double)(c->stat_write_miss) / (double)(c->stat_write_access);
	}

	printf("\n%s_READ_ACCESS    \t\t : %10llu", header, c->stat_read_access);
	printf("\n%s_WRITE_ACCESS   \t\t : %10llu", header, c->stat_write_access);
	printf("\n%s_READ_MISS      \t\t : %10llu", header, c->stat_read_miss);
	printf("\n%s_WRITE_MISS     \t\t : %10llu", header, c->stat_write_miss);
	printf("\n%s_READ_MISS_PERC  \t\t : %10.3f", header, 100*read_mr);
	printf("\n%s_WRITE_MISS_PERC \t\t : %10.3f", header, 100*write_mr);
	printf("\n%s_DIRTY_EVICTS   \t\t : %10llu", header, c->stat_dirty_evicts);

	printf("\n");
}


/////////////////////////////////////////////////////////////////////////////////////
// Allocate memory for the data structures 
// Initialize the required fields 
/////////////////////////////////////////////////////////////////////////////////////

Temporary_Cache* temporary_cache_new(uint64_t size, uint64_t assoc, uint64_t linesize, uint64_t repl_policy){
	//We need to define size, assoc, linesize, repl_policy
	//We need to allocate memory for the cache
	//void *calloc(size_t nitems, size_t size) where nitem is number of items and size is the size of the item
	
	//We allocated memory for the cache
	Temporary_Cache *temporary_cache = (Temporary_Cache *) malloc (1 * sizeof (Temporary_Cache));

	//We have cache_sets, num_sets, repl_policy, num_ways
	//Cache size = (associativity)(number of sets)(block size)
	//This means number of sets is cache size / (block size) * (associtiativy)
	temporary_cache->number_sets = size / (linesize * assoc);
	//We need to also allocate memory for the cache set
	temporary_cache->replacement_policy = repl_policy; 
	//Number of ways is associativity 
	temporary_cache->number_ways = assoc;
	return temporary_cache;
	
}
Utility_Monitor_Struct* utility_monitor_struct_new(uint64_t size, uint64_t assoc, uint64_t linesize, uint64_t repl_policy){
	//We need to define size, assoc, linesize, repl_policy
	//We need to allocate memory for the cache
	//void *calloc(size_t nitems, size_t size) where nitem is number of items and size is the size of the item
	
	//We allocated memory for the cache
	Utility_Monitor_Struct *utility_monitor = (Utility_Monitor_Struct *) malloc (1 * sizeof (Utility_Monitor_Struct));

	//We have cache_sets, num_sets, repl_policy, num_ways
	//Cache size = (associativity)(number of sets)(block size)
	//This means number of sets is cache size / (block size) * (associtiativy)
	utility_monitor->Auxiliary_Tag_Directory = cache_new(size, utility_monitor->number_ways, linesize,utility_monitor->replacement_policy);
	utility_monitor->number_sets = size / (linesize * assoc);
	//We need to also allocate memory for the cache set
	utility_monitor->replacement_policy = repl_policy; 
	//Number of ways is associativity 
	utility_monitor->number_ways = assoc;
	return utility_monitor;
	
}

Cache* cache_new(uint64_t size, uint64_t assoc, uint64_t linesize, uint64_t repl_policy){
	//We need to define size, assoc, linesize, repl_policy
	//We need to allocate memory for the cache
	//void *calloc(size_t nitems, size_t size) where nitem is number of items and size is the size of the item
	
	//We allocated memory for the cache
	Cache *cache = (Cache *) malloc (1 * sizeof (Cache));

	//We have cache_sets, num_sets, repl_policy, num_ways
	//Cache size = (associativity)(number of sets)(block size)
	//This means number of sets is cache size / (block size) * (associtiativy)
	cache->number_sets = size / (linesize * assoc);
	//We need to also allocate memory for the cache set
	cache->cache_sets  = (Cache_Set *) malloc (cache->number_sets * sizeof(Cache_Set));
	cache->replacement_policy = repl_policy; 
	//Number of ways is associativity 
	cache->number_ways = assoc;
	if(cache->replacement_policy == 4)
	{
		uint64_t core0 = 0;
		uint64_t core1 = 1;
		cache->utility_monitor_struct[core0] = utility_monitor_struct_new(size, assoc, linesize, repl_policy);
     	cache->utility_monitor_struct[core1] = utility_monitor_struct_new(size, assoc, linesize, repl_policy);
	}
	return cache;
	
}


/////////////////////////////////////////////////////////////////////////////////////
// Return HIT if access hits in the cache, MISS otherwise 
// Also if is_write is TRUE, then mark the resident line as dirty
// Update appropriate stats
/////////////////////////////////////////////////////////////////////////////////////


bool cache_access(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id){
	/*
	is_write is true, then we need to mark it as dirty 
	if we have a hit, then hit 

	Line address is the block address. 
	We know that block address is the byte address / bytes per block
	index = block address / number of blocks
	*/ 
	uint64_t number_of_sets = c->number_sets;
	uint64_t cache_index = (lineaddr % number_of_sets);
	bool is_hit = false;

	uint64_t cache_tag = lineaddr / number_of_sets;
	uint64_t cache_ways = c->number_ways;
	
	if(c->replacement_policy == 5)
  	{
		for(uint64_t ii=0; ii<cache_ways; ii++)
		{
			if ((c->cache_sets[cache_index].cache_line[ii].valid == true) && (c->cache_sets[cache_index].cache_line[ii].tag == cache_tag) && (c->cache_sets[cache_index].cache_line[ii].core_id == 0))
			{
				is_hit = true;
				// Also if is_write is TRUE, then mark the resident line as dirty
				if(is_write)
				{
					c->cache_sets[cache_index].cache_line[ii].dirty = true;
				}
			}
		}

		//Update appropriate stats
		if(is_write)
		{
			c->stat_write_access++;
			if(is_hit == false)
			{
				if(VERBOSE == true)
				{
					std::cout << "stat_write_miss" << c->stat_write_miss << std::endl;
				}
				c->stat_write_miss++;
			}
		}
		else
		{
			c->stat_read_access++;
			if(is_hit == false)
			{
				if(VERBOSE == true)
				{
					std::cout << "stat_read_miss" << c->stat_read_miss << std::endl;
				}
				c->stat_read_miss++;
			}
		}

		// Return HIT if access hits in the cache, MISS otherwise 
		if(is_hit == false)
		{
			return false;
		}
		else
		{
			return true;
		}

		for(uint64_t ii=0; ii<cache_ways; ii++)
		{
			if ((c->cache_sets[cache_index].cache_line[ii].valid == true) && (c->cache_sets[cache_index].cache_line[ii].tag == cache_tag) && (c->cache_sets[cache_index].cache_line[ii].core_id == 1))
			{
				is_hit = true;
				// Also if is_write is TRUE, then mark the resident line as dirty
				if(is_write)
				{
					c->cache_sets[cache_index].cache_line[ii].dirty = true;
				}
			}
		}

		//Update appropriate stats
		if(is_write)
		{
			c->stat_write_access++;
			if(is_hit == false)
			{
				if(VERBOSE == true)
				{
					std::cout << "stat_write_miss" << c->stat_write_miss << std::endl;
				}
				c->stat_write_miss++;
			}
		}
		else
		{
			c->stat_read_access++;
			if(is_hit == false)
			{
				if(VERBOSE == true)
				{
					std::cout << "stat_read_miss" << c->stat_read_miss << std::endl;
				}
				c->stat_read_miss++;
			}
		}

		// Return HIT if access hits in the cache, MISS otherwise 
		if(is_hit == false)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	
	if(c->replacement_policy == 2)
  	{
		SWP_CORE0_WAYS = 12;
	}
	if(c->replacement_policy == 4)
  	{
		uint64_t counter = 0;
		uint64_t partition_cycle = 5000000;
		if(partition_cycle/cycle == 0)
		{
			counter++;
		}
		else
		{
			counter = counter/2;
			counter++;
		}

		uint64_t utility_ways = c->utility_monitor_struct[core_id]->number_ways;
		uint64_t core0;
		uint64_t core1;
		uint64_t ii = 0;
		while(ii<utility_ways)
		{
			if(c->cache_sets[cache_index].cache_line[ii].core_id == 0)
			{

				core0++;
			}
			if(c->cache_sets[cache_index].cache_line[ii].core_id == 1)
			{
				core1++;
			}
			ii++;
		}
		

		for(uint64_t ii=0; ii<cache_ways; ii++)
		{
			
			if ((c->cache_sets[cache_index].cache_line[ii].valid == true) && (c->cache_sets[cache_index].cache_line[ii].tag == cache_tag))
			{
				is_hit = true;
				// Also if is_write is TRUE, then mark the resident line as dirty
				if(is_write)
				{
					c->cache_sets[cache_index].cache_line[ii].dirty = true;
				}
			}
		}

		//Update appropriate stats
		if(is_write)
		{
			c->stat_write_access++;
			if(is_hit == false)
			{
				if(VERBOSE == true)
				{
					std::cout << "stat_write_miss" << c->stat_write_miss << std::endl;
				}
				c->stat_write_miss++;
			}
		}
		else
		{
			c->stat_read_access++;
			if(is_hit == false)
			{
				if(VERBOSE == true)
				{
					std::cout << "stat_read_miss" << c->stat_read_miss << std::endl;
				}
				c->stat_read_miss++;
			}
		}

		// Return HIT if access hits in the cache, MISS otherwise 
		if(is_hit == false)
		{
			return false;
		}
		else
		{
			return true;
		}

		for(uint64_t ii=0; ii<cache_ways; ii++)
		{
			if ((c->cache_sets[cache_index].cache_line[ii].valid == true) && (c->cache_sets[cache_index].cache_line[ii].tag == cache_tag) && (c->cache_sets[cache_index].cache_line[ii].core_id == 1))
			{
				is_hit = true;
				// Also if is_write is TRUE, then mark the resident line as dirty
				if(is_write)
				{
					c->cache_sets[cache_index].cache_line[ii].dirty = true;
				}
			}
		}

		//Update appropriate stats
		if(is_write)
		{
			c->stat_write_access++;
			if(is_hit == false)
			{
				if(VERBOSE == true)
				{
					std::cout << "stat_write_miss" << c->stat_write_miss << std::endl;
				}
				c->stat_write_miss++;
			}
		}
		else
		{
			c->stat_read_access++;
			if(is_hit == false)
			{
				if(VERBOSE == true)
				{
					std::cout << "stat_read_miss" << c->stat_read_miss << std::endl;
				}
				c->stat_read_miss++;
			}
		}

		// Return HIT if access hits in the cache, MISS otherwise 
		if(is_hit == false)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	for(uint64_t ii=0; ii<cache_ways; ii++)
	{
		if ((c->cache_sets[cache_index].cache_line[ii].valid == true) && (c->cache_sets[cache_index].cache_line[ii].tag == cache_tag) && (c->cache_sets[cache_index].cache_line[ii].core_id == core_id))
		{
			is_hit = true;
			// Also if is_write is TRUE, then mark the resident line as dirty
			if(is_write)
			{
				c->cache_sets[cache_index].cache_line[ii].dirty = true;
			}
		}
	}

	//Update appropriate stats
	if(is_write)
	{
		c->stat_write_access++;
		if(is_hit == false)
		{
			if(VERBOSE == true)
			{
				std::cout << "stat_write_miss" << c->stat_write_miss << std::endl;
			}
			c->stat_write_miss++;
		}
	}
	else
	{
		c->stat_read_access++;
		if(is_hit == false)
		{
			if(VERBOSE == true)
			{
				std::cout << "stat_read_miss" << c->stat_read_miss << std::endl;
			}
			c->stat_read_miss++;
		}
	}

	// Return HIT if access hits in the cache, MISS otherwise 
	if(is_hit == false)
	{
		return false;
	}
	else
	{
		return true;
	}
}


/////////////////////////////////////////////////////////////////////////////////////
// Install the line: determine victim using replacement policy
// Copy victim into last_evicted_line for tracking writebacks
/////////////////////////////////////////////////////////////////////////////////////


void cache_install(Cache* c, Addr lineaddr, uint32_t is_write, uint32_t core_id){

  	// Find victim using cache_find_victim()
	//Fix this !!! number_sets
	uint64_t number_of_sets = c->number_sets;
	uint64_t cache_index = (lineaddr % number_of_sets);
	uint64_t cache_tag = lineaddr / number_of_sets;

	uint64_t victim_way = cache_find_victim(c, cache_index, core_id);

	if(VERBOSE == true)
	{
		std::cout << "victim way" << victim_way << std::endl;
	}

	// Copy victim into last_evicted_line for tracking writebacks
	c->last_evicted_line = c->cache_sets[cache_index].cache_line[victim_way];
	//Update stats
	if (c->cache_sets[cache_index].cache_line[victim_way].dirty == true)
	{
		if(VERBOSE == true)
		{
			std::cout << "dirty evicts" << c->stat_dirty_evicts << std::endl;
		}
		c->stat_dirty_evicts++;
	}

	//Update the other values 
	c->cache_sets[cache_index].cache_line[victim_way].valid = true;
  	c->cache_sets[cache_index].cache_line[victim_way].tag = cache_tag;
	if(is_write) 
	{
		c->cache_sets[cache_index].cache_line[victim_way].dirty = true;
	} 
	else 
	{
		c->cache_sets[cache_index].cache_line[victim_way].dirty = false;
	}
  	c->cache_sets[cache_index].cache_line[victim_way].core_id = core_id;
  	c->cache_sets[cache_index].cache_line[victim_way].insertion_time = cycle;

}


/////////////////////////////////////////////////////////////////////////////////////
// You may find it useful to split victim selection from install
/////////////////////////////////////////////////////////////////////////////////////


uint32_t cache_find_victim(Cache *c, uint32_t set_index, uint32_t core_id)
{
// 0:FIFO 1:SWP (Part E)  2:NEW (Part F) 

	//There is space in the cache  
	uint64_t cache_ways = c->number_ways;
	for(uint64_t ii = 0; ii<cache_ways; ii++)
	{
		if(c->cache_sets[set_index].cache_line[ii].valid == false)
		{	
			return ii;
		}
	}
	//FIFO
	
	if(c->replacement_policy == 0)
	{	
		bool valid = false;
		int fifo_set_index = -1;
		uint64_t oldest_insertion_time = -1; 
		int cache_ways = c->number_ways;

		//Similar implementation to the last lab for finding the oldest instruction time
		int jj = 0;
		while(jj<cache_ways)
		{
			if(c->cache_sets[set_index].cache_line[jj].valid == true)
			{	
				uint32_t new_insertion_time = c->cache_sets[set_index].cache_line[jj].insertion_time;
				if(fifo_set_index == -1 || new_insertion_time < oldest_insertion_time)
				{
					oldest_insertion_time = new_insertion_time;
					fifo_set_index = jj;
					valid = true;
				}
			}
			jj++;
		}
		if(valid == true)
		{
			if(VERBOSE == true)
			{
				std::cout << "set" << set_index << std::endl;
				std::cout << fifo_set_index << std::endl;
			}
			
			return fifo_set_index;
		}
		else
		{
			assert(false);
		}
	}

	//SWP
	if(c->replacement_policy == 1)
	{
		bool valid = false;
		int fifo_set_index = -1;
		uint64_t oldest_insertion_time = -1; 
		int cache_ways = c->number_ways;

		//Based on the lab description
		uint64_t quota_for_core0 = 0;
    	//uint64_t quote_for_core1 = 0;
		quota_for_core0 = SWP_CORE0_WAYS; //4
    	//quote_for_core1 = N - SWP_CORE0_WAYS;
		uint64_t core0 = 0;
		uint64_t core1 = 1;

		//How many core 0 cache lines are there? 
		uint64_t core0_cache_lines = 0;
		uint64_t core1_cache_lines = 0;
		int xx = 0;
		while(xx<cache_ways)
		{
			if(c->cache_sets[set_index].cache_line[xx].core_id == 0)
			{
				core0_cache_lines++;
			}
			if(c->cache_sets[set_index].cache_line[xx].core_id == 1)
			{
				core1_cache_lines++;
			}

			xx++;
		}
		if(VERBOSE == true)
		{
			std::cout << "core cache line " << core0_cache_lines << std::endl;
			std::cout << "cache way count" << xx << std::endl;
		}

		uint64_t selected_core = 0;
		
		if(quota_for_core0 > core0_cache_lines)
		{
			selected_core = core1;
			if(VERBOSE == true)
			{
				std::cout << "1" << std::endl;
			}
		}
		if(quota_for_core0 == core0_cache_lines)
		{
			selected_core = core_id;
		}
		if (quota_for_core0 < core0_cache_lines)
		{
			selected_core = core0;
			if(VERBOSE == true)
			{
				std::cout << "3" << std::endl;
			}
		}
		if(VERBOSE == true)
		{
			std::cout << "selected core " << selected_core << std::endl;
		}

		//Similar implementation to the last lab for finding the oldest instruction time
		int jj = 0;
		while(jj<cache_ways)
		{
			if(c->cache_sets[set_index].cache_line[jj].valid == true)
			{	
				uint32_t new_insertion_time = c->cache_sets[set_index].cache_line[jj].insertion_time;
				if(fifo_set_index == -1 || new_insertion_time < oldest_insertion_time)
				{
					if(c->cache_sets[set_index].cache_line[jj].core_id == selected_core)
					{
						oldest_insertion_time = new_insertion_time;
						fifo_set_index = jj;
						valid = true;
					}
				}
			}
			jj++;
		}
		if(valid == true)
		{
			if(VERBOSE == true)
			{
				std::cout << "set" << set_index << std::endl;
				std::cout << fifo_set_index << std::endl;
			}
			
			return fifo_set_index;
		}
	}

	//NEW
	//Utility based cache partition 
	if(c->replacement_policy == 2)
	{
		bool valid = false;
		int fifo_set_index = -1;
		uint64_t oldest_insertion_time = -1; 
		int cache_ways = c->number_ways;

		//Based on the lab description
		uint64_t quota_for_core0 = 0;
    	//uint64_t quote_for_core1 = 0;
		quota_for_core0 = SWP_CORE0_WAYS; //4
    	//quote_for_core1 = N - SWP_CORE0_WAYS;
		uint64_t core0 = 0;
		uint64_t core1 = 1;

		//How many core 0 cache lines are there? 
		uint64_t core0_cache_lines = 0;
		uint64_t core1_cache_lines = 1;
		int xx = 0;
		while(xx<cache_ways)
		{
			if(c->cache_sets[set_index].cache_line[xx].core_id == 0)
			{
				core0_cache_lines++;
			}
			if(c->cache_sets[set_index].cache_line[xx].core_id == 1)
			{
				core1_cache_lines++;
			}
			xx++;
		}

		uint64_t selected_core = 0;
		if(quota_for_core0 == core0_cache_lines)
		{
			selected_core = core_id;
		}
		if(quota_for_core0 > core0_cache_lines)
		{
			selected_core = core1;
		}
		if(quota_for_core0 < core0_cache_lines)
		{
			selected_core = core0;
		}
		

		//Similar implementation to the last lab for finding the oldest instruction time
		int jj = 0;
		while(jj<cache_ways)
		{
			if(c->cache_sets[set_index].cache_line[jj].valid == true)
			{	
				uint32_t new_insertion_time = c->cache_sets[set_index].cache_line[jj].insertion_time;
				if(fifo_set_index == -1 || new_insertion_time < oldest_insertion_time)
				{
					if(c->cache_sets[set_index].cache_line[jj].core_id == selected_core)
					{
						oldest_insertion_time = new_insertion_time;
						fifo_set_index = jj;
						valid = true;
					}
				}
			}
			jj++;
		}
		if(valid == true)
		{
			if(VERBOSE == true)
			{
				std::cout << "set" << set_index << std::endl;
				std::cout << fifo_set_index << std::endl;
			}
			
			return fifo_set_index;
		} 
	}

	if(c->replacement_policy == 4)
	{
		bool valid = false;
		int fifo_set_index = -1;
		uint64_t oldest_insertion_time = -1; 
		int cache_ways = c->number_ways;

		//Based on the lab description
		uint64_t quota_for_core0 = 0;
    	//uint64_t quote_for_core1 = 0;
		quota_for_core0 = SWP_CORE0_WAYS; //4
    	//quote_for_core1 = N - SWP_CORE0_WAYS;
		uint64_t core0 = 0;
		uint64_t core1 = 1;

		//How many core 0 cache lines are there? 
		uint64_t core0_cache_lines = 0;
		int xx = 0;
		while(xx<cache_ways)
		{
			if(c->cache_sets[set_index].cache_line[xx].core_id == 0)
			{
				core0_cache_lines++;
			}
			xx++;
		}

		uint64_t selected_core = 0;
		if(quota_for_core0 == core0_cache_lines)
		{
			selected_core = core_id;
		}
		if(quota_for_core0 > core0_cache_lines)
		{
			selected_core = core1;
		}
		if(quota_for_core0 < core0_cache_lines)
		{
			selected_core = core0;
		}



		//Similar implementation to the last lab for finding the oldest instruction time
		int jj = 0;
		while(jj<cache_ways)
		{
			if(c->cache_sets[set_index].cache_line[jj].valid == true)
			{	
				uint32_t new_insertion_time = c->cache_sets[set_index].cache_line[jj].insertion_time;
				if(fifo_set_index == -1 || new_insertion_time < oldest_insertion_time)
				{
					if(c->cache_sets[set_index].cache_line[jj].core_id == selected_core)
					{
						oldest_insertion_time = new_insertion_time;
						fifo_set_index = jj;
						valid = true;
					}
				}
			}
			jj++;
		}
		if(valid == true)
		{
			if(VERBOSE == true)
			{
				std::cout << "set" << set_index << std::endl;
				std::cout << fifo_set_index << std::endl;
			}
			
			return fifo_set_index;
		} 
	}
}




