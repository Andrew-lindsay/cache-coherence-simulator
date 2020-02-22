#!/bin/bash

function help_output {
    echo "  file example:"
    echo "      - ./run-script.sh trace1.txt"
    echo "  file with optimisation set"
    echo "      - ./run-script.sh trace1.txt -O"
    exit
}

if [ "$#" -eq 0 ]; then
    echo "Please at least provide a file name!"
    help_output
elif [ "$#" -eq 1 ]; then
    FILE_ARG=$1
elif [ "$#" -eq 2 ]; then
    FILE_ARG=$1
    if [ "$2" == "-O" ]; then 
        OPTIMISE=$2
    else
        echo "Argument not understood for optimisation use -O"
        help_output
    fi
else
    echo "Too many args!"
    help_output
fi

# ==== Compile program ====
make

# echo "RUNNING: ./coherence_sim $FILE_ARG $OPTIMISE"
./coherence_sim $FILE_ARG $OPTIMISE