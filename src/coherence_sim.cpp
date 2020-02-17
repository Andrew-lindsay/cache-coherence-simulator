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

    output_file.open("out_" + file_name + ".txt");

    unsigned long total_accesses = (dir.stats.private_accesses +  dir.stats.remote_accesses + dir.stats.off_chip_accesses);
    unsigned long total_latency = (dir.stats.off_chip_latency + dir.stats.remote_latency + dir.stats.private_latency);

    output_file << "Private-accesses: " << dir.stats.private_accesses << endl;
    output_file << "Remote-accesses: " << dir.stats.remote_accesses << endl;
    output_file << "Off-chip-accesses: " << dir.stats.off_chip_accesses << endl;
    output_file << "Total accesses: " << total_accesses << endl;
    output_file << "Replacement-writebacks: " << dir.stats.replacement_writebacks << endl;
    output_file << "Coherence-writebacks: " << dir.stats.coherence_writebacks << endl;
    output_file << "Invalidations-sent: " << dir.stats.invalidations_sent << endl;
    output_file << "Average-latency: " << 
        (total_latency/ static_cast<double>(total_accesses))
        << endl;
    output_file << "Priv-average-latency: " << dir.stats.private_latency/static_cast<double>(dir.stats.private_accesses) << endl;
    output_file << "Rem-average-latency:  " << dir.stats.remote_latency/static_cast<double>(dir.stats.remote_accesses) << endl;
    output_file << "Off-chip-average-latency: " << dir.stats.off_chip_latency/static_cast<double>(dir.stats.off_chip_accesses) << endl;
    output_file << "Total latency: " << total_latency;

    output_file.close();

}

int main(int argc, char const *argv[])
{
    /* code */
    string file;
    ifstream trace;
    std::string line;
    
    string command_type;

    string proc;
    string operation;
    unsigned int mem_addr;

    Directory dir = Directory();

    //  get trace to run from command line
    if(argc >= 2){
        file = argv[1];
    }else{
        cout << "please provide a trace file\n";
        return -1;
    }

    // read input from file
    trace.open(file);
    
    if(trace.is_open()){

        // util file is empty
        while(trace >> command_type){
            
            if(command_type[0] == 'P'){
                
                proc = command_type;
                trace >> operation;
                trace >> mem_addr; // geting as unsigned integer

                // debug

                // get processor as int
                unsigned int proc_num = proc[1] - 48; // 48 is zero in ascii

                if(proc_num > 3){
                    cerr << "EXITING: Invalid Processor ID of " << proc_num << endl;
                    return -1;
                }else if(operation != "W" && operation != "R"){
                    cerr << "EXITING: Invalid operation W or R only" << endl;
                    return -1;
                }

                cout << "proc: " << proc_num << ", op: " << operation << ", mem_addr: " << mem_addr << endl;

                // use the read values to c
                dir.update(proc_num, mem_addr, operation);

            }else if(command_type[0] == 'p'){
                dir.dump_caches();

            }else if(command_type[0] == 'h'){

                double hit_rate = dir.hit_rate();
                cout << "Current hit rate: " << hit_rate << endl;

            }else if(command_type[0] == 'v'){
                dir.toggle_commentry();
            }else{
                cerr << "malformed line" << endl;
            }
        }

        cout << "File finished" << endl;
        trace.close();

        


        // printing results 
        if(true){
            unsigned long total_accesses = (dir.stats.private_accesses +  dir.stats.remote_accesses + dir.stats.off_chip_accesses);
            unsigned long total_latency = (dir.stats.off_chip_latency + dir.stats.remote_latency + dir.stats.private_latency);

            cout << "Private-accesses: " << dir.stats.private_accesses << endl;
            cout << "Remote-accesses: " << dir.stats.remote_accesses << endl;
            cout << "Off-chip-accesses: " << dir.stats.off_chip_accesses << endl;
            cout << "Total accesses: " << total_accesses << endl;
            cout << "Replacement-writebacks: " << dir.stats.replacement_writebacks << endl;
            cout << "Coherence-writebacks: " << dir.stats.coherence_writebacks << endl;
            cout << "Invalidations-sent: " << dir.stats.invalidations_sent << endl;
            cout << "Average-latency: " << 
                (total_latency/ static_cast<double>(total_accesses))
                << endl;
            cout << "Priv-average-latency: " << dir.stats.private_latency/static_cast<double>(dir.stats.private_accesses) << endl;
            cout << "Rem-average-latency:  " << dir.stats.remote_latency/static_cast<double>(dir.stats.remote_accesses) << endl;
            cout << "Off-chip-average-latency: " << dir.stats.off_chip_latency/static_cast<double>(dir.stats.off_chip_accesses) << endl;
            cout << "Total latency: " << total_latency;
        }

        // write results to file
        result_to_file(dir, file.substr(0, file.length() - 4));

    }else{
        cout << "File failed to open" << endl;
        return -1;
    }

    return 0;
}
