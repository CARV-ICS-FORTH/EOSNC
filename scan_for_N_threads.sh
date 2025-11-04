#!/bin/bash

SAMPLE_NUMBER=32000000

export OMP_PROC_BIND=true
export OMP_PLACES=cores


re='^[0-9]+$'
if ! [[ $1 =~ $re ]] ; then
   echo "error: '$1' is not a number" >&2; exit 1
fi

stat ncar &> /dev/null
if ! [[ $? == 0  ]] ; then
   echo "ncar seems to be missing, make sure you have built it" >&2; exit 1
fi

EXIST=$(./ncar | grep usage)

if [[ -z $EXIST ]]
then
    echo "./ncar fails to execute, are you sure it is compiled correctly? are you perhaps missing openmp libraries?"
    exit 1
fi

for i in $(seq ${1})
do
    echo "running for $i threads"
    rm -rf results_threads_$i
    mkdir results_threads_$i
    OMP_NUM_THREADS=${i} ./ncar ${SAMPLE_NUMBER} > results_threads_$i/status.txt
    mv samples* ./results_threads_$i/
done

mkdir all_results
mv results* all_results
tar -cvf all_results_$1_threads.tar all_results
gzip all_results_$1_threads.tar
rm -rf all_results

