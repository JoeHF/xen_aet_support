#!/bin/sh
benchmark=$1
echo $benchmark
declare -A map=(["fullastar"]="400" ["jason"]="b" ["lee"]="c")
echo ${map[$benchmark]}
while [ 1 ]
do
	sleep 60
	./xc -c 0 -l ${map[$benchmark]} & 
done
