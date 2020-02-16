#ifndef GUARD_CO_SIM
#define GUARD_CO_SIM

#include <vector>
#include <stdio.h>
#include <unordered_map>
#include <iostream>
#include <string>

using std::vector;
using std::unordered_map;
using std::cout;
using std::endl;
using std::string;

enum class cache_state {
    I=0,S,M
};

struct cache_entry{
    unsigned int tag = 1;
   /* State 00 : invalid 
    *       01 : shared
    *       10 : modified*/
    unsigned int state = 0;
};

struct dir_entry{
    unsigned int shared_vec;
    /*State 00 : not cached 
    *       01 : shared
    *       10 : modified*/
    unsigned int state;
};

struct statistic{
    unsigned int private_accesses;
    unsigned int remote_accesses;
    unsigned int off_chip_accesses;
    unsigned int replacement_writebacks;
    unsigned int coherence_writebacks;
    unsigned int invalidations_sent;
    unsigned int private_latency;
    unsigned int remote_latency;
    unsigned int off_chip_latency;
    unsigned int private_latency_ops;
    unsigned int remote_latency_ops;
    unsigned int offchip_latency_ops;
};


class Directory{
    // vector<vector<cache_entry>> caches;
    // Directory(): caches(4, vector<cache_entry>(512, cache_entry())) {}
    cache_entry caches[4][512]; // hold cache tags and state for each line (all 4 processors)

    // don't know how large memory is or how long address are yet (probably 64bits or 32bit)
    unordered_map<unsigned int, dir_entry> dir_entries;

    statistic stats;
    bool commentry = false;
    
public: 
    Directory() {}

    /*
    * Prints contents of processor caches
    */
    void dump_caches(){
        for (int i = 0; i < 4; i++){
            cout << "P" << i << endl;
            for (int j = 0; j < 512; j++){
                cout << j <<  " " << caches[i][j].tag << " "; 
                unsigned int cState = caches[i][j].state;
                if(cState == 0){
                    cout << "I" << endl;
                }else if(cState == 1){
                    cout << "S" << endl;
                }else{
                    cout << "M" << endl;
                }
            }
        }
    }

    void toggle_commentry(){
        
    }

    // what order to deal with the state and operation type 
    void update(int processor, unsigned int address, string operation){

        // if not in dir map add to map



        // if not in cache, ask directory if other procs have it 
            // if no procs have it (pentially for memory access)
                // change state for line in processor
                // change state for line in directory

                // was cache block being replaced valid
            
                // other cache has line 

        // is in cache of processor
            // check state if 

        // 

        int x = caches[0][0].tag;
        printf("tag: %d\n", x);
    }

};

#endif
