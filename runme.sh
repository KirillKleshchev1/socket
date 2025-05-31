#!/bin/bash

gcc server.c -o server
gcc client.c -o client

CONFIG="config.txt"
NUMBERS="numbers.txt"
LOG="/tmp/server.log"

rm -f $CONFIG
rm -f /tmp/server.sock
rm -f $LOG
rm -f $NUMBERS

echo "/tmp/server.sock" >> config.txt
python3 create.py numbers.txt
./server config.txt &
sleep 1

function task1 () {
    local ZERO_CHECK="zero"

    for i in {1..100}; do
        (./client $CONFIG $i 1000 < $NUMBERS) >> /dev/null &
        pids[${i}]=$!
    done

    wait "${pids[@]}"

    echo "finished 100 clients" >> result.txt
    echo "Server state" >> result.txt

    touch $ZERO_CHECK
    echo 0 > $ZERO_CHECK

    ./client $CONFIG $i 1000 < $NUMBERS >> /dev/null
    tail -n 1 $LOG >> result.txt
    rm -f $ZERO_CHECK
}

echo "Task 1" >> result.txt
task1
echo "" >> result.txt

echo "Task 2" >> result.txt
task1
task1

echo "Check heap" >> result.txt
head -n 2 $LOG | tail -n 1 >> result.txt
echo "---" >> result.txt
tail -n 2 $LOG | head -n 1 >> result.txt
echo "" >> result.txt

echo "Task 3" >> result.txt

clients_count=(10 20 30 40 50 60 70 80 90 100)
delays=(0 0.2 0.4 0.6 0.8 1.0)

for count in ${clients_count[@]}; do
    for delay in ${delays[@]}; do
        echo "Test: Counts clients = $count; Delay = $delay" >> result.txt
        SECONDS=0

        for ((i=1; i<=$count; i++)); do
            (./client $CONFIG $i 30 $delay < $NUMBERS) >> /tmp/client.log &
            pids[${i}]=$!
        done

        wait "${pids[@]}"

        client_time=$(cat /tmp/client.log | grep "client time:" | grep -Eo '[0-9]+' | sort -rn | head -n 1)
        echo "Client time = $client_time" >> result.txt

        duration=$SECONDS
        effective_time=$((duration - client_time))
        echo "Total = $duration" >> result.txt
        echo "Effective = $effective_time" >> result.txt
        rm /tmp/client.log
    done
done
echo "" >> result.txt

pkill -f "./server config.txt"
