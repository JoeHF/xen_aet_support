#!/bin/sh
benchmark=$1
echo $benchmark
declare -A map=(["fullastar"]="400" ["fullbzip2"]="500" ["fullbwaves"]="2000" ["fullgcc"]="1000" ["fullmcf"]="2000" ["fullmilc"]="1500" ["fullzeusmp"]="2000" ["fullcactus"]="800" ["fullgems"]="2000" ["fulllbm"]="800" ["fake"]="30" ["fullsoplex"]="400" ["fullpovray"]="2000" ["fullcalculix"]="100" ["fullhmmer"]="100" ["fullsjeng"]="300" ["fulllib"]="100" ["fullh264"]="100" ["fulltonto"]="100" ["fullomnetpp"]="200" ["fullwrf"]="2000" ["fullsphinx3"]="100" ["fullxalan"]="2000") 
echo ${map[$benchmark]}
aet_time=0
sleep 20 
while [ 1 ]
do
	aet_time=$(($aet_time+1))
	./xc -t $aet_time -c 0 -l ${map[$benchmark]} & 
	sleep 10
	#./xc -c 0 -l ${map[$benchmark]} & 
	#./xc -c 0 -l 2000 & 
done
