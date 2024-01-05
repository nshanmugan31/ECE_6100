#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "dram.h"

// As part of the lab description
extern MODE SIM_MODE;
extern uint64_t  CACHE_LINESIZE;
extern bool DRAM_PAGE_POLICY;
////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE PRINT STATS FUNCTION ------------
////////////////////////////////////////////////////////////////////

void dram_print_stats(DRAM* dram){
	double rddelay_avg=0, wrdelay_avg=0;
	char header[256];
	sprintf(header, "DRAM");

	if (dram->stat_read_access) {
		rddelay_avg = (double)(dram->stat_read_delay) / (double)(dram->stat_read_access);
	}

	if (dram->stat_write_access) {
		wrdelay_avg = (double)(dram->stat_write_delay) / (double)(dram->stat_write_access);
	}

	printf("\n%s_READ_ACCESS\t\t : %10llu", header, dram->stat_read_access);
	printf("\n%s_WRITE_ACCESS\t\t : %10llu", header, dram->stat_write_access);
	printf("\n%s_READ_DELAY_AVG\t\t : %10.3f", header, rddelay_avg);
	printf("\n%s_WRITE_DELAY_AVG\t\t : %10.3f", header, wrdelay_avg);

}

//////////////////////////////////////////////////////////////////////////////
// Allocate memory to the data structures and initialize the required fields
//////////////////////////////////////////////////////////////////////////////

DRAM* dram_new() {
	DRAM *dram = (DRAM *) malloc (1 * sizeof (DRAM));
	return dram;
}

//////////////////////////////////////////////////////////////////////////////
// You may update the statistics here, and also call dram_access_mode_CDE()
//////////////////////////////////////////////////////////////////////////////

uint64_t dram_access(DRAM* dram, Addr lineaddr, bool is_dram_write) {
	uint64_t fixed_dram_delay = 100;

	if((SIM_MODE == SIM_MODE_C) | (SIM_MODE == SIM_MODE_D) | (SIM_MODE == SIM_MODE_E))
	{
		fixed_dram_delay = dram_access_mode_CDE(dram, lineaddr, is_dram_write);
	}
	
	if(is_dram_write == true)
	{
		dram->stat_write_access++;
		
		dram->stat_write_delay+=fixed_dram_delay;
	}
	if(is_dram_write == false)
	{
		dram->stat_read_access++;
		dram->stat_read_delay+=fixed_dram_delay;
		//std::cout << "dram read_access " << dram->stat_read_access << std::endl;
	}
	
  
  	return fixed_dram_delay;
}

//////////////////////////////////////////////////////////////////////////////
// Modify the function below only for Parts C,D,E
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Assume a mapping with consecutive cache lines in consecutive DRAM banks
// Assume a mapping with consecutive rowbufs in consecutive DRAM rows
//////////////////////////////////////////////////////////////////////////////
uint64_t dram_access_mode_CDE(DRAM* dram, Addr lineaddr, bool is_dram_write) {
//Row buffer is flushed each time for closed 
//CAS, BUS, 
//If it is a hit for open page then u don't have to close it back 
//Closed page is a static 100 cycle 

	/*
	if(DRAM_PAGE_POLICY = 0)
	{
		uint64_t dram_banks = 16;
		uint64_t row_buffer_size = 1024;
		uint64_t act = 45;
		uint64_t cas = 45;
		uint64_t pre = 45;
		uint64_t bus = 10;

	}
	*/
	uint64_t dram_access_delay = 0;
	uint64_t dram_banks = 16;
	uint64_t cache_linesize = CACHE_LINESIZE;
	uint64_t row_buffer_size = 1024;
	uint64_t act = 45;
	uint64_t cas = 45; //Column Address Strobe
	uint64_t pre = 45;
	uint64_t bus = 10; 
	uint64_t row_buffer_lines = row_buffer_size / cache_linesize;
	uint64_t bank_id = lineaddr % dram_banks; 
	uint64_t row_id = (lineaddr / row_buffer_lines) / dram_banks;

	if(DRAM_PAGE_POLICY == false)
	{
		//Row buffer is not valid 
		//RAS + CAS if array precharged OR
		if (dram->array[bank_id].valid == false)
		{
			dram_access_delay = act + cas + bus;
			dram->array[bank_id].valid = true;
			dram->array[bank_id].row_id = row_id;
		}
		//Row is valid and the id matches 
		//Row buffer hit 
		//Simple CAS if row is“open”
		else if ((dram->array[bank_id].valid == true) && (dram->array[bank_id].row_id == row_id))
		{
			dram_access_delay = cas + bus;
		}
		//PRE + RAS + CAS (worst case)
		else 
		{
			dram_access_delay = pre + act + cas + bus;
			dram->array[bank_id].valid = true;
			dram->array[bank_id].row_id = row_id;
		}
	}
	if(DRAM_PAGE_POLICY == true)
	{
		//Row is valid and the id matches 
		//Row buffer hit 
		if ((dram->array[bank_id].valid == true) && (dram->array[bank_id].row_id == row_id))
		{
			dram_access_delay = cas + act + bus;
		}
		//RAS + CAS
		else 
		{
			dram_access_delay = act + cas + bus;
			dram->array[bank_id].valid = true;
			dram->array[bank_id].row_id = row_id;
		}
	}
  
  return dram_access_delay;

}

