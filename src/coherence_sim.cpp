#include "coherence_sim.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

using std::ifstream;
using std::cout;
using std::cerr;
using std::endl;
using std::string;

// global statics for hits


int main(int argc, char const *argv[])
{
    /* code */
    const char* file = nullptr;
    ifstream trace;
    std::string line;
    
    string command_type;

    string proc;
    string operation;
    string mem_addr;

    Directory dir = Directory();

    // get trace to run from command line
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
                trace >> mem_addr;

                // debug
                cout << "proc: " << proc << ", op: " << operation << ", mem_addr: " << mem_addr << endl;
                // use the read values to c
                dir.update();

            }else if(command_type[0] == 'p'){

            }else if(command_type[0] == 'h'){
                
            }else if(command_type[0] == 'v'){

            }else{
                cerr << "malformed line\n";
            }
        }

        cout << "File finished" << endl;
        trace.close();
    }else{
        cout << "File failed to open" << endl;
        return -1;
    }

    return 0;
}
