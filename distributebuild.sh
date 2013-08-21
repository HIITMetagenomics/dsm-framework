#!/bin/bash
if [ "$#" -ne "3" ]; then
    echo "Expecting parameters: <begin> <adder> <end>"
    exit 1
fi
echo "----------------------------------------------------------------------------"
echo "Running with parameters: <begin=$1> <adder=$2> <end=$3>"
max=$3
adder=$2
i=$1
for node in `cat -`; do
    echo "----------------------------------------------------------------------------"
    echo "Checking the load of ${node}. Clients from $i to $((i+adder)) are pending."
    uptime=`ssh -o BatchMode=yes -o ConnectTimeout=5 -o ServerAliveCountMax=5 -o ServerAliveInterval=3 $node 'cat /proc/loadavg'`
    if [ "$?" -ne "0" ]; then
        echo "FAILED connection to ${node}"
        sleep 1
        continue
    fi
    echo -n "$uptime "
    loadavg=`echo $uptime | awk '{print \$1}'`
    thisloadavg=`echo $loadavg|awk -F \. '{print $1}'`
    if [ "$thisloadavg" -ge "2" ]; then
        echo "Busy - Load Average $loadavg ($thisloadavg) "
    else
        echo "Okay - Load Average $loadavg ($thisloadavg) "
        if [ "$((i+adder))" -ge "$max" ]; then
            let adder=$((max-i))
            echo "Last round detected, remaining adder is ${adder}"
        fi
        set -x
        ssh -f -o BatchMode=yes -o ConnectTimeout=5 -o ServerAliveCountMax=5 -o ServerAliveInterval=3 $node "sh -c \"cd /indexpath; for item in \\\`ls /fastapath/*.fasta.gz | head -n $((i+adder)) | tail -n ${adder}\\\`; do gunzip -c \\\$item > \\\${item##*/}.fasta; ~/metaminer/builder -v \\\${item##*/}.fasta >> \\\${item##*/}.log 2>&1; rm \\\${item##*/}.fasta; done &\""
        set +x
        if [ "$?" -ne "0" ]; then
            echo "FAILED to deliver the command to ${node}"
            sleep 1
            continue
        fi
        
        echo "SUCCESS when delivering the command to ${node}"
        sleep 1
        let i+=adder
    fi

    if [ "$i" -ge "$max" ]; then
        echo "All jobs submitted (i = $i in total)"
        exit 0
    fi
done
echo "Failed to submit all jobs (i = $i submitted)"
exit 1
 