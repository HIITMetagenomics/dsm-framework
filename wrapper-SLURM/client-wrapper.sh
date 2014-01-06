#!/bin/bash
###
### Please use example-client.sh to initiate this script.
###

## name of your job
#SBATCH -J client_%j

## system error message output file
#SBATCH -e client_%j.ER

## system message output file
#SBATCH -o client_%j.OU

## a per-process (soft) memory limit
## limit is specified in MB
## example: 1 GB is 1000
#SBATCH --mem-per-cpu=1000

## how long a job takes, wallclock time hh:mm:ss
#SBATCH -t 00:05:00

# print usage information
if [ "$#" -ne "3" ]; then
   echo "error: Expecting parameters: <sample list> <tmp dir> <index file>"
   echo "error: Expecting parameters: <sample list> <tmp dir> <index file>" > /dev/stderr
   exit 1
fi
if [ -z "$DSM_FRAMEWORK_PATH" ]; then
    echo "error: Please use  export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/"
    echo "error: Please use  export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/" > /dev/stderr
    exit 1
fi  
cat $2/metaserver_config.txt | $DSM_FRAMEWORK_PATH/metaenumerate --fmin 2 -v $3
