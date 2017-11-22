#!/bin/bash

sum=0 nsec=0

function runTest
{  
    CMD=$1

    OUTPUT=`$CMD`
    echo $OUTPUT
    nsec=`echo $OUTPUT | nawk '{ print $6 }'`
    sum=$((sum + nsec))
    return
}

function runRaw
{  
    CMD=$1

    OUTPUT=`$CMD`
    echo $OUTPUT
    return
}

function getAverage
{
    n=$1
    echo average: sum=$sum n=$1
    printf "%.1f\n\n" $(( sum/n ))
    sum=0
    nsec=0
}

# test Message Queue Latency 
###########################################
if [ -x latency_pxmsg ]
then
    echo "run latency_pxmsg 10000 1 ...."
    for i in 1 2 3 4 5
    do
        runTest "./latency_pxmsg 10000 1"
    done
    echo "****"
    getAverage 5
fi

if [ -x latency_pxmsg ]
then
    echo "run latency_pxmsg 10000 64 ...."
    for i in 1 2 3 4 5
    do
        runTest "./latency_pxmsg 10000 64"
    done
    echo "****"
    getAverage 5
fi

if [ -x latency_pxmsg ]
then
    echo "run latency_pxmsg 10000 128 ...."
    for i in 1 2 3 4 5
    do
        runTest "./latency_pxmsg 10000 128"
    done
    echo "****"
    getAverage 5
fi

if [ -x latency_pxmsg ]
then
    echo "run latency_pxmsg 10000 1024 ...."
    for i in 1 2 3 4 5
    do
        runTest "./latency_pxmsg 10000 1024"
    done
    echo "****"
    getAverage 5
fi

if [ -x latency_pxmsg ]
then
    echo "run latency_pxmsg 10000 2048 ...."
    for i in 1 2 3 4 5
    do
        runTest "./latency_pxmsg 10000 2048"
    done
    echo "****"
    getAverage 5
fi

if [ -x latency_pxmsg ]
then
    echo "run latency_pxmsg 10000 4096 ...."
    for i in 1 2 3 4 5
    do
        runTest "./latency_pxmsg 10000 4096"
    done
    echo "****"
    getAverage 5
fi

if [ -x latency_pxmsg ]
then
    echo "run latency_pxmsg 10000 8192 ...."
    for i in 1 2 3 4 5
    do
        runTest "./latency_pxmsg 10000 8192"
    done
    echo "****"
    getAverage 5
fi

# test Message Queue bandwidth 
###########################################
if [ -x bandwidth_pxmsg ]
then
    echo "run bandwidth_pxmsg 5 100 64 ...."
        ./bandwidth_pxmsg 5 100 64
    echo "****"
fi

if [ -x bandwidth_pxmsg ]
then
    echo "run bandwidth_pxmsg 5 100 128 ...."
        ./bandwidth_pxmsg 5 100 128
    echo "****"
fi

if [ -x bandwidth_pxmsg ]
then
    echo "run bandwidth_pxmsg 5 100 1024 ...."
        ./bandwidth_pxmsg 5 100 1024
    echo "****"
fi
