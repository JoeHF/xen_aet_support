#!/bin/sh
benchmark=$1
echo $benchmark
declare -A map=(["fullastar"]="400" ["fullbzip2"]="500" ["fullbwaves"]="2000" ["fullgcc"]="1000" ["fullmcf"]="2000" ["fullmilc"]="1500" ["fullzeusmp"]="2000" ["fullcactus"]="800" ["fullgems"]="2000" ["fulllbm"]="800" ["fake"]="30" ["fullsoplex"]="400" ["fullcalculix"]="500" ["fullhmmer"]="100" ["fullsjeng"]="300" ["fulllib"]="100" ["fullh264"]="100" ["fulltonto"]="100" ["fullomnetpp"]="200" ["fullsphinx3"]="100"]) 
echo ${map[$benchmark]}
aet_time=0
sleep 10
while [ 1 ]
do
	sleep 10 
	aet_time=$(($aet_time+1))
	./xc -t $aet_time -c 0 -l ${map[$benchmark]} & 
	#./xc -c 0 -l ${map[$benchmark]} & 
	#./xc -c 0 -l 2000 & 
done
