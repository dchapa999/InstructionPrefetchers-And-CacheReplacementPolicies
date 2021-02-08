//Working SHiP++
#include "cache.h"

#include <cstdlib>
#include <ctime>
#include <cstdint>

#define SAT_INC(x,max)  (x<max)?x+1:x
#define SAT_DEC(x)      (x>0)?x-1:x
#define TRUE 1
#define FALSE 0

#define maxRRPV 3
#define SHCT_SIZE  16384
#define SHCT_PRIME 16381
#define SAMPLER_SET (256*NUM_CPUS)
#define SAMPLER_WAY L1I_WAY
#define SHCT_MAX 7

#define RRIP_OVERRIDE_PERC   0

// The base policy is SRRIP. SHIP needs the following on a per-line basis
uint32_t line_rrpv[L1I_SET][L1I_WAY];
uint32_t is_prefetch[L1I_SET][L1I_WAY];
uint32_t fill_core[L1I_SET][L1I_WAY];

// These two are only for sampled sets (we use 64 sets)
#define NUM_LEADER_SETS   64

uint32_t ship_sample[L1I_SET];
uint32_t line_reuse[L1I_SET][L1I_WAY];
uint64_t line_sig[L1I_SET][L1I_WAY];
	
// SHCT. Signature History Counter Table
// per-core 16K entry. 14-bit signature = 16k entry. 3-bit per entry
#define maxSHCTR 7
//#define SHCT_SIZE (1<<14)


// prediction table structure
class SHCT_class {
  public:
    uint32_t counter;

    SHCT_class() {
        counter = 0;
    };
};
SHCT_class SHCT[NUM_CPUS][SHCT_SIZE];
uint32_t SHCT2[NUM_CPUS][SHCT_SIZE];


// Statistics
uint64_t insertion_distrib[NUM_TYPES][maxRRPV+1];
uint64_t total_prefetch_downgrades;

uint32_t rrpv[L1I_SET][L1I_WAY];

// sampler structure
class SAMPLER_class
{
  public:
    uint8_t valid,
            type,
            used;

    uint64_t tag, cl_addr, ip;
    
    uint32_t lru;

    SAMPLER_class() {
        valid = 0;
        type = 0;
        used = 0;

        tag = 0;
        cl_addr = 0;
        ip = 0;

        lru = 0;
    };
};

// sampler
uint32_t rand_sets[SAMPLER_SET];
SAMPLER_class sampler[SAMPLER_SET][SAMPLER_WAY];

// initialize replacement state
void CACHE::l1i_initialize_replacement()
{

    for (int i=0; i<L1I_SET; i++) {
        for (int j=0; j<L1I_WAY; j++) {
            line_rrpv[i][j] = maxRRPV;
            line_reuse[i][j] = FALSE;
            is_prefetch[i][j] = FALSE;
            line_sig[i][j] = 0;
        }
    }

    for (int i=0; i<NUM_CPUS; i++) {
        for (int j=0; j<SHCT_SIZE; j++) {
            SHCT[i][j].counter = 1; // Assume weakly re-use start
        }
    }

    int leaders=0;

    while(leaders<NUM_LEADER_SETS){
      int randval = (rand() % L1I_SET);
      
      if(ship_sample[randval]==0){
        ship_sample[randval]=1;
        leaders++;
      }
    }

}

// find replacement victim
uint32_t CACHE::l1i_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    // look for the maxRRPV line
    while (1)
    {
        for (int i=0; i<L1I_WAY; i++)
            if (rrpv[set][i] == maxRRPV)
                return i;

        for (int i=0; i<L1I_WAY; i++)
            rrpv[set][i]++;
    }

    // WE SHOULD NOT REACH HERE
    assert(0);
    return 0;
}

// called on every cache hit and cache fill
void CACHE::l1i_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    uint32_t sig = line_sig[set][way];

    //cout << "here" << endl;

    if (hit) { // update to REREF on hit
        if( type != WRITEBACK ) 
        {

            if( (type == PREFETCH) && is_prefetch[set][way] )
            {
                if( (ship_sample[set] == 1) && ((rand()%100)<5)) 
                {
                    uint32_t fill_cpu = fill_core[set][way];

                    SHCT2[fill_cpu][sig] = SAT_INC(SHCT2[fill_cpu][sig], maxSHCTR);
                    line_reuse[set][way] = TRUE;
                }
            }
            else 
            {
                line_rrpv[set][way] = 0;

                if( is_prefetch[set][way] )
                {
                    line_rrpv[set][way] = maxRRPV;
                    is_prefetch[set][way] = FALSE;
                    total_prefetch_downgrades++;
                }

                if( (ship_sample[set] == 1) && (line_reuse[set][way]==0) ) 
                {
                    uint32_t fill_cpu = fill_core[set][way];

                    SHCT2[fill_cpu][sig] = SAT_INC(SHCT2[fill_cpu][sig], maxSHCTR);
                    line_reuse[set][way] = TRUE;
                }
            }
        }
        
	    return;
    }
    
    //--- All of the below is done only on misses -------
    // remember signature of what is being inserted
    uint64_t use_PC = (type == PREFETCH ) ? ((PC << 1) + 1) : (PC<<1);
    uint32_t new_sig = use_PC%SHCT_SIZE;
    
    if( ship_sample[set] == 1 ) 
    {
        uint32_t fill_cpu = fill_core[set][way];
        
        // update signature based on what is getting evicted
        if (line_reuse[set][way] == FALSE) { 
            SHCT2[fill_cpu][sig] = SAT_DEC(SHCT2[fill_cpu][sig]);
        }
        else 
        {
            SHCT2[fill_cpu][sig] = SAT_INC(SHCT2[fill_cpu][sig], maxSHCTR);
        }

        line_reuse[set][way] = FALSE;
        line_sig[set][way]   = new_sig;  
        fill_core[set][way]  = cpu;
    }



    is_prefetch[set][way] = (type == PREFETCH);

    // Now determine the insertion prediciton

    uint32_t priority_RRPV = maxRRPV-1 ; // default SHIP

    if( type == WRITEBACK )
    {
        line_rrpv[set][way] = maxRRPV;
    }
    else if (SHCT2[cpu][new_sig] == 0) {
      line_rrpv[set][way] = (rand()%100>=RRIP_OVERRIDE_PERC)?  maxRRPV: priority_RRPV; //LowPriorityInstallMostly
    }
    else if (SHCT2[cpu][new_sig] == 7) {
        line_rrpv[set][way] = (type == PREFETCH) ? 1 : 0; // HighPriority Install
    }
    else {
        line_rrpv[set][way] = priority_RRPV; // HighPriority Install 
    }

    // Stat tracking for what insertion it was at
    insertion_distrib[type][line_rrpv[set][way]]++;
}

// use this function to print out your own stats at the end of simulation
void CACHE::l1i_replacement_final_stats()
{
    
}
