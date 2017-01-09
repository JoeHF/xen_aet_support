#!/bin/sh
benchmark=$1
echo $benchmark
declare -A map=(["fullastar"]="400" ["fullbzip2"]="1000" ["fullbwaves"]="1000" ["fullgcc"]="1000" ["fullmcf"]="2000" ["fullmilc"]="1000" ["fullzeusmp"]="1000" ["fullcactus"]="2000" ["fullgems"]="2000" ["fulllbm"]="800") 
echo ${map[$benchmark]}
while [ 1 ]
do
	sleep 60
	#./xc -c 0 -l ${map[$benchmark]} & 
	./xc -c 0 -l 2000 & 
done
