#!/bin/bash
###
### builder job script example
###

## name of your job
#SBATCH -J builder_%j

## system error message output file
#SBATCH -e builder_%j.ER

## system message output file
#SBATCH -o builder_%j.OU

## a per-process (soft) memory limit
## limit is specified in MB
## example: 1 GB is 1000
#SBATCH --mem-per-cpu=1000

## how long a job takes, wallclock time hh:mm:ss
#SBATCH -t 00:02:00

# print usage information
if [ "$#" -ne "1" ]; then
   echo "error: Expecting parameters: <input file>"
   echo "error: Expecting parameters: <input file>" > /dev/stderr
   exit 1
fi
if [ -z "$DSM_FRAMEWORK_PATH" ]; then
    echo "error: Please use  export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/"
    echo "error: Please use  export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/" > /dev/stderr
    exit 1
fi  
$DSM_FRAMEWORK_PATH/builder -v $1
