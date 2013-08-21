#!/bin/bash
if [ "$#" -ne "7" ]; then
    echo "Expecting parameters: <begin> <adder> <end> <host> <port> <fmin> <path>"
    exit 1
fi
echo "----------------------------------------------------------------------------"
echo "Running with parameters: <begin=$1> <adder=$2> <end=$3> <host=$4> <port=$5> <fmin=$6> <path=$7>"
max=$3
adder=$2
i=$1
host=$4
port=$5
fmin=$6
epath=$7
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
        ssh -o BatchMode=yes -o ConnectTimeout=5 -o ServerAliveCountMax=5 -o ServerAliveInterval=3 $node "cd /outputpath; for item in \`head -n $((i+adder)) list_of_input_files.txt | tail -n ${adder}\`; do ~/metaminer/metaenumerate --port $port --fmin $fmin --path $epath -v /indexpath/\${item} $host > \${item}.log 2>&1 & done"
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
 