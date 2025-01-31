#include "coherence_sim.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <assert.h>

using std::ifstream;
using std::ofstream;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

// global statics for hits

void result_to_file(Directory dir, string file_name){

    ofstream output_file;

    output_file.open("out_" + file_name);
    // cout << "out_" + file_name << endl;

    if(output_file.is_open()){
        unsigned long total_accesses = (dir.stats.private_accesses +  dir.stats.remote_accesses + dir.stats.off_chip_accesses);
        unsigned long total_latency = (dir.stats.off_chip_latency + dir.stats.remote_latency + dir.stats.private_latency);

        output_file << "Private-accesses: " << dir.stats.private_accesses << "\n";
        output_file << "Remote-accesses: " << dir.stats.remote_accesses << "\n";
        output_file << "Off-chip-accesses: " << dir.stats.off_chip_accesses << "\n";
        output_file << "Total-accesses: " << total_accesses << "\n";
        output_file << "Replacement-writebacks: " << dir.stats.replacement_writebacks << "\n";
        output_file << "Coherence-writebacks: " << dir.stats.coherence_writebacks << "\n";
        output_file << "Invalidations-sent: " << dir.stats.invalidations_sent << "\n";
        output_file << "Average-latency: " << 
            (total_latency/ static_cast<double>(total_accesses))
            << "\n";
        output_file << "Priv-average-latency: " << dir.stats.private_latency/static_cast<double>(dir.stats.private_accesses) << "\n";
        output_file << "Rem-average-latency:  " << dir.stats.remote_latency/static_cast<double>(dir.stats.remote_accesses) << "\n";
        output_file << "Off-chip-average-latency: " << dir.stats.off_chip_latency/static_cast<double>(dir.stats.off_chip_accesses) << "\n";
        output_file << "Total-latency: " << total_latency << endl;

        output_file.close();
    }else{
        cerr << "ERROR: Failed to open output file." << endl;
    }
}

int main(int argc, char const *argv[])
{
    /* code */
    string file;
    ifstream trace;
    string line;
    
    string command_type;

    string proc;
    string operation;
    unsigned int mem_addr;

    bool optimised = false;

    //  get trace to run from command line
    if(argc == 2){
        file = argv[1];
    }else if(argc > 2){
        file = argv[1];
        if(string(argv[2]) == "-O"){
            optimised = true;
        }else{
            cerr << "ERROR: was expecting optimised arg -O got " << argv[2] << endl;
            return -1;
        }
    }else{
        cout << "please provide a trace file\n";
        return -1;
    }

    Directory dir = Directory(optimised);

    // read input from file
    trace.open(file);
    
    if(trace.is_open()){

        // util file is empty
        while(trace >> command_type){
            
            if(command_type[0] == 'P'){
                
                proc = command_type;
                trace >> operation;
                trace >> mem_addr; // geting as unsigned integer

                // get processor as int
                unsigned int proc_num = proc[1] - 48; // 48 is zero in ascii

                if(proc_num > 3){
                    cerr << "EXITING: Invalid Processor ID of " << proc_num << endl;
                    return -1;
                }else if(operation != "W" && operation != "R"){
                    cerr << "EXITING: Invalid operation W or R only" << endl;
                    return -1;
                }

                // debug
                //cout << "proc: " << proc_num << ", op: " << operation << ", mem_addr: " << mem_addr << endl;

                // send parameters to directory to simulate caches
                dir.update(proc_num, mem_addr, operation);

            }else if(command_type[0] == 'p'){
                dir.dump_caches();

            }else if(command_type[0] == 'h'){

                double hit_rate = dir.hit_rate();
                cout << "Current hit rate: " << hit_rate << endl;

            }else if(command_type[0] == 'v'){
                dir.toggle_commentry();
            }else{
                cerr << "ERROR: Malformed line" << endl;
            }
        }

        // cout << "File finished" << endl;
        trace.close();

        // printing results to terminal long with additional statics    
        if(false){
            unsigned long total_accesses = (dir.stats.private_accesses +  dir.stats.remote_accesses + dir.stats.off_chip_accesses);
            unsigned long total_latency = (dir.stats.off_chip_latency + dir.stats.remote_latency + dir.stats.private_latency);

            cout << "Private-accesses: " << dir.stats.private_accesses << "\n";
            cout << "Remote-accesses: " << dir.stats.remote_accesses << "\n";
            cout << "Off-chip-accesses: " << dir.stats.off_chip_accesses << "\n";
            cout << "Total-accesses: " << total_accesses << endl;
            cout << "Replacement-writebacks: " << dir.stats.replacement_writebacks << "\n";
            cout << "Coherence-writebacks: " << dir.stats.coherence_writebacks << "\n";
            cout << "Invalidations-sent: " << dir.stats.invalidations_sent << "\n";
            cout << "Average-latency: " << 
                (total_latency/ static_cast<double>(total_accesses))
                << "\n";
            cout << "Priv-average-latency: " << dir.stats.private_latency/static_cast<double>(dir.stats.private_accesses) << "\n";
            cout << "Rem-average-latency:  " << dir.stats.remote_latency/static_cast<double>(dir.stats.remote_accesses) << "\n";
            cout << "Off-chip-average-latency: " << dir.stats.off_chip_latency/static_cast<double>(dir.stats.off_chip_accesses) << "\n";
            cout << "Total-latency: " << total_latency << "\n";
            cout << "Private-latency: " << dir.stats.private_latency << "\n";
            cout << "Remote-latency: " << dir.stats.remote_latency << "\n";
            cout << "Off-chip-latency: " << dir.stats.off_chip_latency << "\n";
            cout << "Total-Blocks-replaced-locally: " << dir.stats.block_replacement << "\n";
            cout << "Local-write-shared-state: " << dir.stats.local_write_shared_state << "\n";
            cout << "Remote_writes_to_invalid_state: " << dir.stats.remote_writes_to_invalid_state << "\n";
            cout << "Remote_reads_to_invalid_state: " << dir.stats.remote_reads_to_invalid_state << "\n";
            cout << "Single-sharer-write-to-share: " << dir.stats.single_sharer_write << endl;
        }

        // write results to file
        result_to_file(dir, file.substr(file.find_last_of("/") + 1));

    }else{
        cerr << "File failed to open" << endl;
        return -1;
    }

    return 0;
}
