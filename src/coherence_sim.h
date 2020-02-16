#ifndef GUARD_CO_SIM
#define GUARD_CO_SIM

#include <vector>
#include <stdio.h>
#include <unordered_map>
#include <iostream>
#include <string>
#include <assert.h>
#include <cmath>
#include <algorithm>

using std::vector;
using std::unordered_map;
using std::cout;
using std::endl;
using std::string;

#define NUM_PROCS 4
#define MAX(x,y) (x < y ? y : x)

enum cache_state {
    I=0,S,M
};

enum dir_state{
    Invalid=0,Cached,Modified
};

struct cache_entry{
    int tag = -1;
   /* State 00 : (I) invalid 
    *       01 : (S) shared
    *       10 : (M) modified*/
    cache_state state = cache_state::I;
};

struct dir_entry{
    bool shared_vec[4] = {false, false, false, false};  
    /*State 00 : not cached
    *       01 : shared
    *       10 : modified*/
    dir_state state = dir_state::Invalid;
};

struct statistic{
    unsigned int private_accesses       = 0;
    unsigned int remote_accesses        = 0;
    unsigned int off_chip_accesses      = 0;
    unsigned int replacement_writebacks = 0;
    unsigned int coherence_writebacks   = 0; 
    unsigned int invalidations_sent     = 0;
    unsigned int private_latency        = 0;
    unsigned int remote_latency         = 0;
    unsigned int off_chip_latency       = 0;
    // unsigned int private_latency_ops;
    // unsigned int remote_latency_ops;
    // unsigned int offchip_latency_ops;
};


class Directory{
    // vector<vector<cache_entry>> caches;
    // Directory(): caches(4, vector<cache_entry>(512, cache_entry())) {}
    cache_entry caches[4][512]; // hold cache tags and state for each line (all 4 processors)

    // don't know how large memory is or how long address are yet (probably 64bits or 32bit)
    unordered_map<unsigned int, dir_entry> dir_entries;

    statistic stats;
    bool commentry = false; // used for turning on commentry
    
public: 
    Directory() {}

    //
    bool is_all_false(bool vec[], int n){
        bool all = false;
        for(int i = 0; i < n; i++){
            all = all || vec[i]; 
        }
        return !all;
    }

    /*
    * Prints contents of processor caches
    */
    void dump_caches(){
        for (int i = 0; i < 4; i++){
            cout << "P" << i << endl;
            for (int j = 0; j < 512; j++){
                cout << j <<  " " << caches[i][j].tag << " "; 
                unsigned int cState = caches[i][j].state;
                if(cState == cache_state::I){
                    cout << "I" << endl;
                }else if(cState == cache_state::S){
                    cout << "S" << endl;
                }else{
                    cout << "M" << endl;
                }
            }
        }
    }

    /*
    * toggle printing of verbose message output    
    */
    void toggle_commentry(){
        commentry = !commentry;
    }

    /*
    * Calculates hit rate
    */
    double hit_rate(){
        unsigned int total = stats.remote_accesses 
            + stats.off_chip_accesses + stats.private_accesses; 
        return (static_cast<double>(stats.private_accesses)) / total;
    }

    // what order to deal with the state and operation type 
    void update(int processor, unsigned int address, string operation){

        // if not in dir map add to map 
        if(dir_entries.find(address) == dir_entries.end()){
            dir_entries[address];
        }

        // calculate tag and index, (offset is 2 bits as there are 4 word to a cache line)
        unsigned int index = (address >> 2) & 511; // 0x1ff
        unsigned int tag  = address >> 11; // 2 bits offset + 9 bits index (bit shift away 11 bits to be left with the tag)

        // DEBUG
        cout << "tag: " << tag << " index: " << index << endl;
        assert(index < 512);

        cache_entry lcache_entry = caches[processor][index];

        // ========== CHECK CACHES AND DIRECTORY ============

        // move from share to modified if write occurs here

        if(lcache_entry.tag == tag && lcache_entry.state != cache_state::I /*invalid*/){ // check local cache
            cout << "hit in local cache" << endl;

            stats.private_accesses++; //private accesses as in local cache

            // combinations of operation and cache line state

            // check if read or write operation ?
            if( operation == "R" && (lcache_entry.state == cache_state::S || lcache_entry.state == cache_state::M) ){
                
                // don't have to update state of local line or directory
                stats.private_latency += 1/* cache probe*/ + 1 /* cache read or write access*/;

            }else if(operation == "W"  && lcache_entry.state == cache_state::M){
                
                stats.private_latency += 1/* cache probe*/ + 1 /* cache read or write access*/;

            }else if( operation == "W" && lcache_entry.state == cache_state::S){
                 
                stats.private_latency += 1; // prob cache

                stats.private_latency += 3; // proc tell dir to invalidate other caches

                stats.private_latency += 3; // directory sends message to processors to invalidate overlap in time
                
                stats.private_latency += 1; // processors probe caches and invalidate

                int hops = 0;
                for(int i = 0; i < NUM_PROCS; i++){
                    if( i != processor && dir_entries[address].shared_vec[i] == true){
                        dir_entries[address].shared_vec[i] = false; // directory
                        caches[i][index].state = cache_state::I; // change cache state for ones sharing value
                        
                        stats.invalidations_sent++;

                        // if one of them is in modified do we have to have a write back
                        // no as the state is shared not modifed

                        // get greatest distance between processors
                        hops = std::max(abs(processor - i), hops);
                    }
                }
                
                // cal longest path for acks of invalidate to return
                stats.private_latency += hops*3; // invalidate messages returning

                // update directory state ? 
                dir_entries[address].state = dir_state::Modified/*modified*/;
                // ensure sharing vector is correct
                // don't need to update tag here
            }
        }else if( false && dir_entries[address].state != dir_state::Invalid){ // if not in cache, ask directory if other procs have its
            cout << "Hit in other Processor" << endl;
            stats.remote_accesses++;

            // 


        }else{ // have to access main memory
            cout << "Going to memory " << endl;
            stats.off_chip_accesses++;

            stats.off_chip_latency += 1; // probe 

            stats.off_chip_latency += 3; // request to dir to see if other

            stats.off_chip_latency += 10; // 10 cycles to go get data from memory

            stats.off_chip_latency += 3; // send data to processor

            stats.off_chip_latency += 1; // write data to local cache

            // this should incurr a write back fee if block is not invalid
            if(lcache_entry.tag != tag && lcache_entry.state != cache_state::I){

                cout << "BLOCK CLASH" << endl;

                // write back to memory
                if(lcache_entry.state == cache_state::M){
                    stats.replacement_writebacks++;
                    cout << "replaced block " << endl;
                }

                //rebuild address
                unsigned int old_address = ((lcache_entry.tag << 9) | index) << 2;

                // directory update sharing of vec of removed address
                for (int i = 0; i < 4; i++){
                    dir_entries[old_address + i].shared_vec[processor] = false;
                    // have to update directory state here if all are false thens set invalid

                    bool not_shared = is_all_false(dir_entries[old_address+i].shared_vec,4); 
                    if(not_shared){ // if address was only cached in the process make invalid
                        dir_entries[old_address+i].state = dir_state::Invalid;
                    }
                }
            }

            // set states
            caches[processor][index].tag = tag;
            if(operation == "R"){
                caches[processor][index].state = cache_state::S;
                dir_entries[address].state = dir_state::Cached;
            }else{
                caches[processor][index].state = cache_state::M;
                dir_entries[address].state = dir_state::Modified;
            }
        }


        // if not in cache, ask directory if other procs have its

            // if no procs have it (pentially for memory access)
                // change state for line in processor
                // change state for line in directory

                // was cache block being replaced valid
            
                // other cache has line 

        // is in cache of processor
            // check state if 

        // 

        // int x = caches[0][0].tag;
        // printf("tag: %d\n", x);
    }

};

#endif
