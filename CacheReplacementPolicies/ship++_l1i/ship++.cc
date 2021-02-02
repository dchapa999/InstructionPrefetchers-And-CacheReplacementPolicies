#include "cache.h"

#include <cstdlib>
#include <ctime>
#include <cstdint>

#define MAX_LLC_SETS 8192
#define LLC_WAYS 16

#define SAT_INC(x,max)  (x<max)?x+1:x
#define SAT_DEC(x)      (x>0)?x-1:x
#define TRUE 1
#define FALSE 0

#define maxRRPV 3
#define SHCT_SIZE  16384
#define SHCT_PRIME 16381
#define SAMPLER_SET (256*NUM_CPUS)
#define SAMPLER_WAY LLC_WAY
#define SHCT_MAX 7

#define RRIP_OVERRIDE_PERC   0

uint64_t get_cycle_count(), get_instr_count(uint32_t cpu), get_config_number();

// The base policy is SRRIP. SHIP needs the following on a per-line basis
#define maxRRPV 3
uint32_t line_rrpv[MAX_LLC_SETS][LLC_WAYS];
uint32_t is_prefetch[MAX_LLC_SETS][LLC_WAYS];
uint32_t fill_core[MAX_LLC_SETS][LLC_WAYS];

// These two are only for sampled sets (we use 64 sets)
#define NUM_LEADER_SETS   64

uint32_t ship_sample[MAX_LLC_SETS];
uint32_t line_reuse[MAX_LLC_SETS][LLC_WAYS];
uint64_t line_sig[MAX_LLC_SETS][LLC_WAYS];
	
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

uint32_t rrpv[LLC_SET][LLC_WAY];

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
    cout << "Initialize SRRIP state" << endl;

    for (int i=0; i<LLC_SET; i++) {
        for (int j=0; j<LLC_WAY; j++) {
            rrpv[i][j] = maxRRPV;
        }
    }
}

// check if this set is sampled
uint32_t is_it_sampled(uint32_t set)
{
    for (int i=0; i<SAMPLER_SET; i++)
        if (rand_sets[i] == set)
            return i;

    return SAMPLER_SET;
}

// update sampler
void update_sampler(uint32_t cpu, uint32_t s_idx, uint64_t address, uint64_t ip, uint8_t type)
{
    SAMPLER_class *s_set = sampler[s_idx];
    uint64_t tag = address / (64*LLC_SET); 
    int match = -1;

    // check hit
    for (match=0; match<SAMPLER_WAY; match++)
    {
        if (s_set[match].valid && (s_set[match].tag == tag))
        {
            uint32_t SHCT_idx = s_set[match].ip % SHCT_PRIME;
            if (SHCT[cpu][SHCT_idx].counter > 0)
                SHCT[cpu][SHCT_idx].counter--;

            /*
            if (draw_transition)
                printf("cycle: %lu SHCT: %d ip: 0x%llX SAMPLER_HIT cl_addr: 0x%llX page: 0x%llX block: %ld set: %d\n", 
                ooo_cpu[cpu].current_cycle, SHCT[cpu][SHCT_idx].dead, s_set[match].ip, address>>6, address>>12, (address>>6) & 0x3F, s_idx);
            */

            //s_set[match].ip = ip; // SHIP does not update ip on sampler hit
            s_set[match].type = type; 
            s_set[match].used = 1;
            //D(printf("sampler hit  cpu: %d  set: %d  way: %d  tag: %x  ip: %lx  type: %d  lru: %d\n",
            //            cpu, rand_sets[s_idx], match, tag, ip, type, s_set[match].lru));

            break;
        }
    }

    // check invalid
    if (match == SAMPLER_WAY)
    {
        for (match=0; match<SAMPLER_WAY; match++)
        {
            if (s_set[match].valid == 0)
            {
                s_set[match].valid = 1;
                s_set[match].tag = tag;
                s_set[match].ip = ip;
                s_set[match].type = type;
                s_set[match].used = 0;

                //D(printf("sampler invalid  cpu: %d  set: %d  way: %d  tag: %x  ip: %lx  type: %d  lru: %d\n",
                //            cpu, rand_sets[s_idx], match, tag, ip, type, s_set[match].lru));
                break;
            }
        }
    }

    // miss
    if (match == SAMPLER_WAY)
    {
        for (match=0; match<SAMPLER_WAY; match++)
        {
            if (s_set[match].lru == (SAMPLER_WAY-1)) // Sampler uses LRU replacement
            {
                if (s_set[match].used == 0)
                {
                    uint32_t SHCT_idx = s_set[match].ip % SHCT_PRIME;
                    if (SHCT[cpu][SHCT_idx].counter < SHCT_MAX)
                        SHCT[cpu][SHCT_idx].counter++;

                    /*
                    if (draw_transition)
                        printf("cycle: %lu SHCT: %d ip: 0x%llX SAMPLER_MISS cl_addr: 0x%llX page: 0x%llX block: %ld set: %d\n", 
                        ooo_cpu[cpu].current_cycle, SHCT[cpu][SHCT_idx].dead, s_set[match].ip, address>>6, address>>12, (address>>6) & 0x3F, s_idx);
                    */
                }

                s_set[match].tag = tag;
                s_set[match].ip = ip;
                s_set[match].type = type;
                s_set[match].used = 0;

                //D(printf("sampler miss  cpu: %d  set: %d  way: %d  tag: %x  ip: %lx  type: %d  lru: %d\n",
                //            cpu, rand_sets[s_idx], match, tag, ip, type, s_set[match].lru));
                break;
            }
        }
    }

    // update LRU state
    uint32_t curr_position = s_set[match].lru;
    for (int i=0; i<SAMPLER_WAY; i++)
    {
        if (s_set[i].lru < curr_position)
            s_set[i].lru++;
    }
    s_set[match].lru = 0;
}

// find replacement victim
uint32_t CACHE::l1i_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t PC, uint64_t full_addr, uint32_t type)
{
    // look for the maxRRPV line
    while (1)
    {
        for (int i=0; i<LLC_WAYS; i++)
            if (line_rrpv[set][i] == maxRRPV) { // found victim
                return i;
            }

        for (int i=0; i<LLC_WAYS; i++)
            line_rrpv[set][i]++;
    }

    // WE SHOULD NOT REACH HERE
    assert(0);
    return 0;
}

// called on every cache hit and cache fill
void CACHE::l1i_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t PC, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    uint32_t sig   = line_sig[set][way];

    if (hit) { // update to REREF on hit
        if( type != WRITEBACK ) 
        {

            if( (type == PREFETCH) && is_prefetch[set][way] )
            {
//                line_rrpv[set][way] = 0;
                //|| (get_config_number()==4))
                if( (ship_sample[set] == 1) && ((rand()%100 <5) )) 
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

string names[] = 
{
    "LOAD", "RFO", "PREF", "WRITEBACK" 
};

// use this function to print out your own stats at the end of simulation
void CACHE::l1i_replacement_final_stats()
{
    cout<<"Insertion Distribution: "<<endl;
    for(uint32_t i=0; i<NUM_TYPES; i++) 
    {
        cout<<"\t"<<names[i]<<" ";
        for(uint32_t v=0; v<maxRRPV+1; v++) 
        {
            cout<<insertion_distrib[i][v]<<" ";
        }
        cout<<endl;
    }

    cout<<"Total Prefetch Downgrades: "<<total_prefetch_downgrades<<endl;
}
