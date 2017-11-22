#!/bin/bash

sum=0 nsec=0

function runClient
{  
    CMD=$1

    OUTPUT=`$CMD`
    echo $OUTPUT
    nsec=`echo $OUTPUT | nawk '{ print $6 }'`
    sum=$((sum + nsec))
    return
}

function runServer
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

# test Unix Domain socket Latency 
###########################################
if [ -x latency_unixsocket_server ]
then
    echo "run latency_unixsocket_server 1 ...."
    for i in 1 2 3 4 5
    do
        ./latency_unixsocket_server 10000 1 &
        runClient "./latency_unixsocket_client 10000 1"
    done
    echo "****"
    getAverage 5
fi

if [ -x latency_unixsocket_server ]
then
    echo "run latency_unixsocket_server 64 ...."
    for i in 1 2 3 4 5
    do
        ./latency_unixsocket_server 10000 64 &
        runClient "./latency_unixsocket_client 10000 64"
    done
    echo "****"
    getAverage 5
fi

if [ -x latency_unixsocket_server ]
then
    echo "run latency_unixsocket_server 128 ...."
    for i in 1 2 3 4 5
    do
        ./latency_unixsocket_server 10000 128 &
        runClient "./latency_unixsocket_client 10000 128"
    done
    echo "****"
    getAverage 5
fi

if [ -x latency_unixsocket_server ]
then
    echo "run latency_unixsocket_server 1024 ...."
    for i in 1 2 3 4 5
    do
        ./latency_unixsocket_server 10000 1024 &
        runClient "./latency_unixsocket_client 10000 1024"
    done
    echo "****"
    getAverage 5
fi

# test Unix Domain socket Bandwidth 
###########################################
if [ -x bandwidth_unixsocket ]
then
    echo "run bandwidth_unixsocket 5 100 64 ...."
        ./bandwidth_unixsocket 5 100 64
    echo "****"
fi

if [ -x bandwidth_unixsocket ]
then
    echo "run bandwidth_unixsocket 5 100 128 ...."
        ./bandwidth_unixsocket 5 100 64
    echo "****"
fi

if [ -x bandwidth_unixsocket ]
then
    echo "run bandwidth_unixsocket 5 100 1024 ...."
        ./bandwidth_unixsocket 5 100 64
    echo "****"
fi