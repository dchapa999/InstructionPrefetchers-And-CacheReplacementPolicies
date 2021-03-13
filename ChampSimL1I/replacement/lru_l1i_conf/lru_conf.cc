#include "cache.h"
#include <algorithm>
#include <iterator>
#include <string>
#include "champsim_constants.h"
#include "util.h"
#include <iostream>

// initialize replacement state
void CACHE::l1i_initialize_replacement()
{

}

// find replacement victim
uint32_t CACHE::l1i_find_victim(uint32_t cpu, uint64_t instr_id, uint32_t set, const BLOCK *current_set, uint64_t ip, uint64_t full_addr, uint32_t type)
{
    // baseline LRU
    return std::distance(current_set, std::max_element(current_set, std::next(current_set, NUM_WAY), lru_comparator<BLOCK, BLOCK>()));
}

// called on every cache hit and cache fill
void CACHE::l1i_update_replacement_state_conf(uint32_t cpu, uint32_t set, uint32_t way, uint64_t full_addr, uint64_t ip, uint64_t victim_addr, uint32_t type, uint8_t hit, double conf)
{
    std::cout <<  "Addr: " << full_addr << "\nType: " << type << "\nConfidence value: " << conf << endl;
    std::string TYPE_NAME;
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

    // uncomment this line to see the LLC accesses
    // cout << "CPU: " << cpu << "  LLC " << setw(9) << TYPE_NAME << " set: " << setw(5) << set << " way: " << setw(2) << way;
    // cout << hex << " paddr: " << setw(12) << paddr << " ip: " << setw(8) << ip << " victim_addr: " << victim_addr << dec << endl;

    if (hit && type == WRITEBACK)
        return;

    auto begin = std::next(block.begin(), set*NUM_WAY);
    auto end   = std::next(begin, NUM_WAY);
    uint32_t hit_lru = std::next(begin, way)->lru;
    std::for_each(begin, end, [hit_lru](BLOCK &x){ if (x.lru <= hit_lru) x.lru++; });
    std::next(begin, way)->lru = 0; // promote to the MRU position
}

void CACHE::l1i_replacement_final_stats()
{

}
