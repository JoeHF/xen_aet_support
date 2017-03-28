./xc -x 8
benchmark=("fullgems" "fullmilc" "fullmcf" "fullcactus" "fullsoplex" "fakestage")
#benchmark=("fullmilc")
for bench in ${benchmark[*]}
do
	sample_rate=32
	echo $bench
	for i in $(seq 0 0)
	do
		echo $i
		if [ $i -eq 0 ] ; then
			sample_rate=1
		else
			#sample_rate=$(($i*256))
			sample_rate=$(($sample_rate*2))
		fi
		echo $sample_rate
		./xc -x $sample_rate
		sh run_fake_pmu.sh $bench 
		sleep 1
		if [ ! -d "/new2/temp_sample_$sample_rate" ] ; then
			mkdir "/new2/temp_sample_$sample_rate"
		fi	
		rm -rf "/new2/temp_sample_$sample_rate/$bench"
		mv "temp/$bench" "/new2/temp_sample_$sample_rate"
	done
done	
	
	#sh run_fake_pmu.sh fullmcf 
	#sleep 10
	#sh run_fake_pmu.sh fullmilc 
	#sleep 10
	#sh run_fake_pmu.sh fullzeusmp 
	#sleep 10
	#sh run_fake_pmu.sh fullcactus
	#sleep 10
	#sh run_fake_pmu.sh fullgems
	#sleep 10
	#sh run_fake_pmu.sh fulllbm
	#sleep 10
	#sh run_fake_pmu.sh fullsoplex
	#sleep 10
	#sh run_fake_pmu.sh fullsjeng 
	#sleep 10
	#sh run_fake_pmu.sh fullomnetpp 
