#!/bin/bash
###
### Please use example-server.sh to initiate this script.
###

## name of your job
#SBATCH -J server_%j

## system error message output file
#SBATCH -e server_%j.ER

## system message output file
#SBATCH -o server_%j.OU

## a per-process (soft) memory limit
## limit is specified in MB
## example: 1 GB is 1000
#SBATCH --mem-per-cpu=1000

## how long a job takes, wallclock time hh:mm:ss
#SBATCH -t 00:05:00

# print usage information
if [ "$#" -ne "5" ]; then
   echo "error: Expecting parameters: <sample list> <tmp dir> <hash> <port> <entropy>"
   echo "error: Expecting parameters: <sample list> <tmp dir> <hash> <port> <entropy>" > /dev/stderr
   exit 1
fi
if [ -z "$DSM_FRAMEWORK_PATH" ]; then
    echo "error: Please use  export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/"
    echo "error: Please use  export DSM_FRAMEWORK_PATH=/path/to/dsm-framework/" > /dev/stderr
    exit 1
fi  
echo -e "$HOSTNAME\t$4\t$3" > $2/metaserver_config_$3.txt
cat $1 | $DSM_FRAMEWORK_PATH/metaserver -p $4 --emax $5 -v --debug | gzip - > server-output.$3.txt.gz
