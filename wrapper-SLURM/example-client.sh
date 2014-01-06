#!/bin/bash
###
### Example client-size initialization.
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
PATH_TO_INDEX_FILES="./"

#Combine the config files
cat $2/metaserver_config_*.txt > $2/metaserver_config.txt

#############################################################################################
# Initialize the client via the wrapper-script.
for i in `cat $1`
do
    sbatch $DSM_FRAMEWORK_PATH/wrapper-SLURM/client-wrapper.sh $1 $2 $PATH_TO_INDEX_FILES/$i.fasta.fmi
done
