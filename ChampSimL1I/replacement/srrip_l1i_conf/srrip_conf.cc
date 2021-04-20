#include "cache.h"

#define maxRRPV 3
/*#define conf0 0.0 // V5 V7 V9
#define conf1 0.25
#define conf2 0.5
#define conf3 0.75
/*#define conf0 0.0 // V5 V7
#define conf1 0.01
#define conf2 0.1
#define conf3 0.5*/
#define conf0 0.0
#define conf1 0.001
#define conf2 0.01
#define conf3 0.1 // V4
uint32_t rrpv[L1I_SET][L1I_WAY];

// initialize replacement state
void CACHE::l1i_initialize_replacement()
{
    cout << "Initialize SRRIP state" << endl;

    for (int i=0; i<L1I_SET; i++) {
        for (int j=0; j<L1I_WAY; j++) {
            rrpv[i][j] = maxRRPV;
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
void CACHE::l1i_update_replacement_state_conf(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit, double conf)
{
    //if (type == PREFETCH)
      //  cout << "Prob: " << conf << endl;


    string TYPE_NAME;
    if (type == LOAD)
        TYPE_NAME = "LOAD";
    else if (type == RFO)
        TYPE_NAME = "RFO";
    else if (type == PREFETCH)
        TYPE_NAME = "PF";
    else if (type == WRITEBACK)
        TYPE_NAME = "WB";
    else
        assert(0);

    if (hit)
        TYPE_NAME += "_HIT";
    else
        TYPE_NAME += "_MISS";

    if ((type == WRITEBACK) && ip)
        assert(0);

    // uncomment this line to see the L1I accesses
    // cout << "CPU: " << cpu << "  L1I " << setw(9) << TYPE_NAME << " set: " << setw(5) << set << " way: " << setw(2) << way;
    // cout << hex << " paddr: " << setw(12) << paddr << " ip: " << setw(8) << ip << " victim_addr: " << victim_addr << dec << endl;
    
    //Custom rrpv confidence value setting 
    //Initially places prefetch misses with rrpv based on confidence value from prefetcher
    if(type == PREFETCH){
        if(!hit){ //V12
            if(conf <= conf1){
                rrpv[set][way] = 1;
            }
            else if((conf > conf1) && (conf <= conf2)){
                rrpv[set][way] = 1;
            }
            else if((conf > conf2) && (conf <= conf3)){
                rrpv[set][way] = 0;
            }
            else if(conf > conf3){
                rrpv[set][way] = 0;
            }
        }
        /*if(!hit){ //V9 V11
            if(conf <= conf1){
                rrpv[set][way] = maxRRPV;
            }
            else if((conf > conf1) && (conf <= conf2)){
                rrpv[set][way] = maxRRPV-1;
            }
            else if((conf > conf2) && (conf <= conf3)){
                rrpv[set][way] = maxRRPV-1;
            }
            else if(conf > conf3){
                rrpv[set][way] = maxRRPV-2;
            }
        }
        /*if(!hit){ //V7
            if(conf <= conf1){
                rrpv[set][way] = 3;
            }
            else if((conf > conf1) && (conf <= conf2)){
                rrpv[set][way] = 2;
            }
            else if((conf > conf2) && (conf <= conf3)){
                rrpv[set][way] = 1;
            }
            else if(conf > conf3){
                rrpv[set][way] = 0;
            }
        }*/
        /*if(!hit){ //V4
            if(conf <= conf1){
                rrpv[set][way] = 2;
            }
            else if((conf > conf1) && (conf <= conf2)){
                rrpv[set][way] = 2;
            }
            else if((conf > conf2) && (conf <= conf3)){
                rrpv[set][way] = 1;
            }
            else if(conf > conf3){
                rrpv[set][way] = 0;
            }
        }*/
        /*if(!hit){ //V3
            if(conf <= conf1){
                rrpv[set][way] = 2;
            }
            else if((conf > conf1) && (conf <= conf2)){
                rrpv[set][way] = 1;
            }
            else if((conf > conf2) && (conf <= conf3)){
                rrpv[set][way] = 0;
            }
            else if(conf > conf3){
                rrpv[set][way] = 0;
            }
        }*/
        /*if(!hit){ //V2
            if(conf <= conf1){
                rrpv[set][way] = 3;
            }
            else if((conf > conf1) && (conf <= conf2)){
                rrpv[set][way] = 2;
            }
            else if((conf > conf2) && (conf <= conf3)){
                rrpv[set][way] = 1;
            }
            else if(conf > conf3){
                rrpv[set][way] = 0;
            }
        }*/
        else{//Decrements rrpv with high confidence to remain in the cache for a longer period of time
            if((conf >= conf2) && (rrpv[set][way] > 0)){
                rrpv[set][way]--;
            }// V1-V5
            //rrpv[set][way]=3;//V6
        }
        
    }
    else{
        if (hit)
            rrpv[set][way] = 0;
        else
            rrpv[set][way] = maxRRPV-1;
    }
}

void CACHE::l1i_update_replacement_state(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit)
{
    string TYPE_NAME;
    if (type == LOAD)
        TYPE_NAME = "LOAD";
    else if (type == RFO)
        TYPE_NAME = "RFO";
    else if (type == PREFETCH)
        TYPE_NAME = "PF";
    else if (type == WRITEBACK)
        TYPE_NAME = "WB";
    else
        assert(0);

    if (hit)
        TYPE_NAME += "_HIT";
    else
        TYPE_NAME += "_MISS";

    if ((type == WRITEBACK) && ip)
        assert(0);

    // uncomment this line to see the L1I accesses
    // cout << "CPU: " << cpu << "  L1I " << setw(9) << TYPE_NAME << " set: " << setw(5) << set << " way: " << setw(2) << way;
    // cout << hex << " paddr: " << setw(12) << paddr << " ip: " << setw(8) << ip << " victim_addr: " << victim_addr << dec << endl;
    
    if (hit)
        rrpv[set][way] = 0;
    else
        rrpv[set][way] = maxRRPV-1;
}

// use this function to print out your own stats at the end of simulation
void CACHE::l1i_replacement_final_stats()
{

}
