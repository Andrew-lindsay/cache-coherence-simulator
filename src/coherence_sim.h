#ifndef GUARD_CO_SIM
#define GUARD_CO_SIM

#include <vector>
#include <stdio.h>
#include <unordered_map>

using std::vector;
using std::unordered_map;

struct cache_entry{
    unsigned int tag = 1;
    /*State 00 : invalid 
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

class Directory{
    cache_entry caches[4][512];
    // don't know how many 
    unordered_map<unsigned int, dir_entry> dir_entries;

    // vector<vector<cache_entry>> caches;
    // Directory(): caches(4, vector<cache_entry>(512, cache_entry())) {}

public: 
    Directory() {}


    void dump_cache(){
        
    }

    void toggle_commentry(){
        
    }

    void update(){

        // if not in dir map add to map

        // if not in cache, ask directory if other procs have it 
            // if no procs have it (pentially for memory access)
                // change state for line in processor
                // change state for line in directory

                // was cache block being replaced valid
            
                // other cache has line 

        // is in cache of processor
            // check state if 

        int x = caches[0][0].tag;
        printf("tag: %d\n", x);
    }

};

#endif
