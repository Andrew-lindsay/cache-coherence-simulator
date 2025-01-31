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
#include <sstream>

using std::vector;
using std::unordered_map;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::stringstream;
using std::to_string;

#define NUM_PROCS 4
#define MAX(x,y) (x < y ? y : x)

enum cache_state {
    I=0,
    S,
    M,
    E
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
    unsigned int block_replacement      = 0;
    unsigned int local_write_shared_state = 0;
    unsigned int remote_reads_to_invalid_state = 0;
    unsigned int remote_writes_to_invalid_state = 0;
    unsigned int single_sharer_write = 0;
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

    bool commentry = false; // used for turning on commentry

                                // 0  1  2  3
    int proc_layout_dist[4][4] = {{0, 1, 2, 1},  // 0
                                  {1, 0, 1, 2},  // 1
                                  {2, 1, 0, 1},  // 2
                                  {1, 2, 1, 0 }};// 3

    string output_message;

    bool optimised = false;

    bool optimised_mesi = false;

public: 
    Directory(){}
    Directory(bool opt): optimised_mesi(opt){}

    statistic stats;
    

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
                if(caches[i][j].tag != -1){
                    cout << j <<  " " << caches[i][j].tag << " "; 
                    unsigned int cState = caches[i][j].state;
                    if(cState == cache_state::I){
                        cout << "I" << endl;
                    }else if(cState == cache_state::S){
                        cout << "S" << endl;
                    }else if(cState == cache_state::E){
                        cout << "E" << endl;
                    }else{
                        cout << "M" << endl;
                    }
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

    void print_commentry(int processor, unsigned int address, string operation){
        cout << "a " <<  ( operation == "R" ? "read" : "write") 
        << " to processor " << processor 
        <<  " to word " << address << endl;
    }

    string state_to_string(cache_state block_state){
        string state_c;
        switch (block_state){
            case cache_state::I : state_c = "Invalid";   break;
            case cache_state::S : state_c = "Shared";    break;
            case cache_state::M : state_c = "Modified";  break;
            case cache_state::E : state_c = "Exclusive"; break;
        }
        return state_c;
    }


    bool only_sharer(bool vec[], int n, int proc){
        bool no_sharers = true;
        for (int i = 0; i < n; i++){
            if(i != proc){
                no_sharers = no_sharers && (vec[i] == false);
            }
        }

        if(vec[proc] == false){
            cerr << "ERROR share vector should be populated" << endl; 
        }
        
        return no_sharers;
    }


    void local_cache_block_replacement(cache_entry lcache_entry, unsigned int tag, unsigned int index, int processor){

        // replace block if cache line is valid
        if(lcache_entry.tag != tag && lcache_entry.state != cache_state::I){

            // cout << "BLOCK CLASH" << endl;
            stats.block_replacement++;
            
            if(commentry){
                output_message += ", local cache block at index " + to_string(index) + " with tag " + to_string(lcache_entry.tag) + " replaced";
            }

            // write back to memory
            if(lcache_entry.state == cache_state::M){
                stats.replacement_writebacks++;

                //cout << "replaced block " << endl;
                
                if(commentry){
                    output_message += " (block written back to memory).";
                }
            }

            // get old address that is being replaced 
            unsigned int old_cache_addr;
            if(optimised){
                old_cache_addr = (lcache_entry.tag << 8) | index; // 8 bit index in optimised
            }else{
                old_cache_addr = (lcache_entry.tag << 9) | index;
            }

            // un share the cache line
            dir_entries[old_cache_addr].shared_vec[processor] = false;

            // directory update sharing of vec of removed address
            bool not_shared = is_all_false(dir_entries[old_cache_addr].shared_vec, 4); 
            if(not_shared){ // if address was only cached of processor make invalid
                dir_entries[old_cache_addr].state = dir_state::Invalid;
            }
        }
    }

    // what order to deal with the state and operation type 
    void update(int processor, unsigned int address, string operation){
        // “A read by processor P2 to word 17 looked for tag 0 in cache-line/block 2, was found in state Invalid (cache miss)
        //  in this cache and found in state Modified in the cache of P1.”

        if(commentry){
            string op = (operation == "R" ? "read" : "write");
            output_message +=  "a " +  op + " to processor " + std::to_string(processor) 
                +  " to word " + std::to_string(address);
        }

        unsigned int cache_line_address;
        if(optimised){
            cache_line_address = address >> 3; // 3 bit offset 8 words per block
        }else{
            cache_line_address = address >> 2; // 2 bit offset 4 words per block
        }

        // if not in dir map add to map 
        if(dir_entries.find(cache_line_address) == dir_entries.end()){
            dir_entries[cache_line_address];
        }

        unsigned int index;
        unsigned int tag;

        if(optimised){
            // calculate tag and index, (offset is 2 bits as there are 4 word to a cache line)
            index = (address >> 3) & 255; // 0x1ff
            tag  = address >> 11; // 2 bits offset + 9 bits index (bit shift away 11 bits to be left with the tag)
        }else{
                        // calculate tag and index, (offset is 3 bits as there are 8 word to a cache line)
            index = (address >> 2) & 511; // 0x1ff
            tag  = address >> 11;
        }

        // DEBUG
        // cout << "tag: " << tag << " index: " << index << endl;

        assert(index < 512);

        cache_entry lcache_entry = caches[processor][index];

        // ========== CHECK CACHES AND DIRECTORY ============

        if(commentry){
            output_message += " looked for tag " + std::to_string(tag) + " in cache block " + to_string(index);
        }

        // ==================== Hit in local cache =====================================
        if(lcache_entry.tag == tag && lcache_entry.state != cache_state::I /*invalid*/){ // check local cache
            //cout << "hit in local cache" << endl;

            if(commentry){
                output_message += " found in state " + state_to_string(lcache_entry.state) + " (cache hit)"; 
            }

            // check if read or write operation ?
            if( operation == "R" && (lcache_entry.state == cache_state::S || lcache_entry.state == cache_state::M || lcache_entry.state == cache_state::E) ){ // add MESI state check here
                stats.private_accesses++; //private accesses as in local cache

                // don't have to update state of local line or directory
                stats.private_latency += 1/* cache probe*/ + 1 /* cache read or write access*/;

            }else if(operation == "W"  && lcache_entry.state == cache_state::M){ 

                stats.private_accesses++; //private accesses as in local cache

                stats.private_latency += 1/* cache probe*/ + 1 /* cache read or write access*/;

                // write in modified state stay in modified state don't update anything

            }else if( operation == "W" && lcache_entry.state == cache_state::S){ // 11 or 14 if in exclusive state this does not need to call directory
                // if no sharers reduce latency ? 

                stats.local_write_shared_state++;

                if( only_sharer(dir_entries[cache_line_address].shared_vec, 4, processor)){
                    stats.single_sharer_write++; // will be zero when optimisation enabled
                } 

                stats.remote_accesses++; //remote accesses as in local cache

                stats.remote_latency += 1; // prob cache

                stats.remote_latency += 3; // proc tell dir to invalidate other caches

                stats.remote_latency += 3; // directory sends message to processors to invalidate overlap in time
                                            // also sends message to processor who request saying how many acks to expecy

                int hops = 0; // stays zero if no other processor shares data
                for(int i = 0; i < NUM_PROCS; i++){
                    if( i != processor && dir_entries[cache_line_address].shared_vec[i] == true){
                        dir_entries[cache_line_address].shared_vec[i] = false; // update sharing vec
                        caches[i][index].state = cache_state::I; // change cache state for ones sharing value

                        stats.invalidations_sent++;

                        if(commentry){
                            output_message += "(P" +  to_string(1) + ")";
                        }

                        // if one of them is in modified do we have to have a write back
                        // no as the state is shared not modifed

                        // get greatest distance between processors
                        hops = std::max(proc_layout_dist[processor][i] , hops); // access map of processors to get cost of jump
                    }
                }

                if(hops != 0){ // we know at least one other processor has the data
                    stats.remote_latency += 1; // processors probe caches and invalidate
                }
                
                // cal longest path for acks of invalidate to return
                stats.remote_latency += hops*3; // invalidate messages returning

                // update directory state ? 

                stats.remote_latency += 1; //Write data to the local cache

                dir_entries[cache_line_address].state = dir_state::Modified/*modified*/;
                caches[processor][index].state = cache_state::M;

                // ensure sharing vector is correct
                dir_entries[cache_line_address].shared_vec[processor] = true; // not need as should be already false
                
                // don't need to update tag here
            }else if(operation == "W" && lcache_entry.state == cache_state::E){

                stats.remote_accesses++;

                stats.remote_latency += 1; // prob cache

                stats.remote_latency += 3; // proc tell dir to update state from cached to modified

                // stats.remote_latency += 1 // write data to cache overlaps

                dir_entries[cache_line_address].state = dir_state::Modified/*modified*/;
                caches[processor][index].state = cache_state::M;


            }else{ // ADD MESI state check here for write in Exclusive state moves to modified state silently 
                cerr << "ERROR: unreachable" <<  endl; 
            }

        // ===================== REMOTE ACCESS ===============================
        }else if(dir_entries[cache_line_address].state != dir_state::Invalid){ // if not in cache, ask directory if other procs have its
            
            //cout << "Hit in other Processor" << endl;
            
            stats.remote_accesses++;

            if(commentry){
                output_message += " found in state Invalid (Cache miss) in local cache";
            }

            // other processors have cached the values
                // what stat is remote line in need to check tag ensure correct? use directory

            if(operation == "R"){ // either remote processors in modifed or shared

                stats.remote_reads_to_invalid_state++;

                stats.remote_latency += 1; //probing local cache for tag

                stats.remote_latency += 3; // send message to directory to req data 

                stats.remote_latency += 3; // directory asks nearest proc to send data

                stats.remote_latency += 1;  // nearest proc prob cache

                stats.remote_latency += 1; // nearest proc accesses cache to get data

                // find CLOSTEST processor that has the data 
                int hops = 3; // stays at 3 if no other processor shares data#
                int remote_proc = 0;
                for(int i = 0; i < NUM_PROCS; i++){
                    if(i != processor && dir_entries[cache_line_address].shared_vec[i] == true){
                        
                        if(commentry){
                            output_message += " and found in state " + state_to_string(caches[i][index].state) 
                                + " in cache of P" + to_string(i) + ",";
                        }

                        int dist = proc_layout_dist[processor][i];
                        if( dist < hops){
                            hops = dist;
                            remote_proc=i; // 
                        }
                    }
                }
                // hops should never be left at 3
                if(hops == 3){cerr << "ERROR on remote read: should have at least one sharer" << endl;}

                stats.remote_latency += hops*3; // send data to processor that requested 

                stats.remote_latency += 1; // processor reads data from cache

                // updated sharing list
                dir_entries[cache_line_address].shared_vec[processor] = true;

                local_cache_block_replacement(lcache_entry, tag, index, processor);
                
                // update local cache values
                caches[processor][index].state = cache_state::S;
                caches[processor][index].tag = tag;
                
                if(dir_entries[cache_line_address].state == dir_state::Modified) { // add MESI state check here as well 
                                                                                   // not needed as on write moves to modified state
                    // change to shared
                    stats.coherence_writebacks++; // remote block in Modified state and a readmiss occurs
                }

                // ensures if cache is in exclusive state it moves to shared on remote read miss
                caches[remote_proc][index].state = cache_state::S; // modifed goes to shared
                dir_entries[cache_line_address].state = dir_state::Cached;

            }else if(operation == "W"){

                stats.remote_writes_to_invalid_state++;

                stats.remote_latency += 1; //probing local cache for tag

                stats.remote_latency += 3; // send message to directory to req data 

                stats.remote_latency += 3; // send message to closest processor with data to forward data and invalidate
                                           // other processors get invalidate messages here 
                                           // processor gets number of invalidates to receive as well

                stats.remote_latency += 1; // proc with data probe cache and tag
                                           // other proc invalidate

                // send invalidate acks
                int hops_max = 0; // should never stay zero we have sharers if we have reached here
                int hops_min = 3; // should never be 3 
                for(int i = 0; i < NUM_PROCS; i++){
                    if( i != processor && dir_entries[cache_line_address].shared_vec[i] == true){
                        dir_entries[cache_line_address].shared_vec[i] = false; // directory

                        if(commentry){
                            output_message += " and found in state " + state_to_string(caches[i][index].state) 
                                + " cache of P" + to_string(i) + ",";
                        }

                        if(caches[i][index].state == cache_state::M){ 

                            stats.coherence_writebacks++; // if in modified state and write occurs writeback triggered
                        }

                        // sets Invalid for Exclusive state as well
                        caches[i][index].state = cache_state::I; // change cache state for ones sharing value
                                                                 // invalidates
                        stats.invalidations_sent++;

                        // get greatest distance between processors
                        hops_max = std::max(proc_layout_dist[processor][i], hops_max); // access map of processors to get cost of jump
                        hops_min = std::min(proc_layout_dist[processor][i], hops_min);
                    }
                }

                if(hops_max == 0 || hops_min == 3){
                    cerr << "ERROR on remote write:  must have at least one sharer" << endl;
                }

                stats.remote_latency += 1; // read data from cache to give to remote proc

                // longer invalidate ack route hides the read to cache to get data for user
                if(hops_min < hops_max){
                    stats.remote_latency -= 1; 
                }

                stats.remote_latency += hops_max*3; // send acks and data to requesting processor

                stats.remote_latency += 1; // processor req data writes data to cache

                // TODO: need to handle local cache replacement and all state information for new cache line

                local_cache_block_replacement(lcache_entry, tag, index, processor);

                // update local cache state
                caches[processor][index].state = cache_state::M;
                caches[processor][index].tag = tag;

                // update directory
                dir_entries[cache_line_address].shared_vec[processor] = true;

                // if directory is already in modified state then we want to write back and set to modified for this processor
                if(dir_entries[cache_line_address].state == dir_state::Modified) { // add MESI check here
                    // change to shared
                    // FIX ME already counted in loop i think

                    //stats.coherence_writebacks++; 
                    // remote block in Modified state and a readmiss occurs (already counted)

                }else{ // if shared set to modified as write has been performed
                    dir_entries[cache_line_address].state = dir_state::Modified;
                }
            }

            // if operation is a write request 

            // update sharing info if cacheline in processor is valid but tags do not match

        // ========================== Off Chip access ================================
        }else{ // have to access main memory
            // cout << "Going to memory " << endl;

            if(commentry){
                output_message += ", not in local cache recieved data from main memory";
            }

            stats.off_chip_accesses++;
            stats.off_chip_latency += 1; // probing local cache for tag and check state
            stats.off_chip_latency += 3; // request to dir to see if other
            stats.off_chip_latency += 10; // 10 cycles to go get data from memory
            stats.off_chip_latency += 3; // send data to processor
            stats.off_chip_latency += 1; // write data to local cache

            // ==== REPLACING OLD CACHE BLOCK ====
            // this should incurr a write back fee if block is not invalid
            
            // if local cache block needs replaced this does the updating of state
            // also write back replacement counted here
            local_cache_block_replacement(lcache_entry, tag, index, processor);    

            // set states
            caches[processor][index].tag = tag;
            if(operation == "R"){
                caches[processor][index].state = cache_state::S;
                dir_entries[cache_line_address].state = dir_state::Cached;
                // IF optimisation enabled then on read move to Exclusive state not
                // caches[processor][index].state = cache_state::E;
                if(optimised_mesi){
                    caches[processor][index].state = cache_state::E;
                }
            }else{
                caches[processor][index].state = cache_state::M;
                dir_entries[cache_line_address].state = dir_state::Modified;
            }

            dir_entries[cache_line_address].shared_vec[processor] = true;
        }

        // output verbose messaging
        if(commentry){
            cout << output_message << endl;
        }
        //reset output message
        output_message.clear();
    }
};

#endif
