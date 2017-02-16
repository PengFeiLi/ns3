#!/usr/bin/env bash

RADIUS=140
SMALLDIST=10
STARTINTERVAL=50

for n in 4 8 20 40
do
    for i in 10 50 100
    do
		for rate in 200000 500000
		do
	        START=$(expr $i \* 50 / 1000 + 20)
    	    SIMT=6 # $(expr $START + 30)
			PKTINTER=$(expr 8 \* 1000000 / $rate)
        	echo "$n $i $rate"
	        ./waf --run "scratch/lena-single-energy -numberOfSmalls=${n} -numberOfUes=${i} -simTime=$SIMT -startTime=$START -smallDist=$SMALLDIST -radius=$RADIUS -rateRequire=$rate -pktInterval=$PKTINTER -startInterval=$STARTINTERVAL"
    	    DIR="data_"${n}"_"${i}"_"${rate}
        	cp -r "data" $DIR
		done
    done
done
