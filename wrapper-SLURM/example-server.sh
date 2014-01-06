#!/bin/bash
###
### Example server-size initialization.
###

# print usage information
if [ "$#" -ne "2" ]; then
   echo "error: Expecting parameters: <sample list> <tmp dir>"
   echo "error: Expecting parameters: <sample list> <tmp dir>" > /dev/stderr
   exit 1
fi
if [ -z "$DSM_FRAMEWORK_PATH" ]; then
    echo "error: Please use  export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/"
    echo "error: Please use  export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/" > /dev/stderr
    exit 1
fi  

#############################################################################################
# Initialize the default settings
# using TCP port numbers starting from
PORT=52000
# entropy cutoff
ENTROPY_CUTOFF=1.2

#############################################################################################
## construct an array of hash values (one for each server process, here 4 processes)
hash=(`for x in A C G T; do echo "$x"; done`)
## here for 16 processes
## hash=(`for x in A C G T; do for y in A C G T; do echo "$x$y"; done; done`)
## here for 64 processes
## hash=(`for x in A C G T; do for y in A C G T; do for z in A C G T; do echo "$x$y$z"; done; done; done`)

#############################################################################################
# Initialize the server via the wrapper-script.
# The wrapper-script is used to track the hostnames running the server processes.
for i in "${hash[@]}"
do
    # Please see the server-wrapper.sh file for SLURM settings...
    sbatch $DSM_FRAMEWORK_PATH/wrapper-SLURM/server-wrapper.sh $1 $2 $i $PORT $ENTROPY_CUTOFF
    let PORT+=1
done
