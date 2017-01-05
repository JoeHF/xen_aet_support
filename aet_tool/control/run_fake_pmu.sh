#!/bin/sh
tmp_file=tmp.txt
XC_DIR=./
PMU_DIR=./
if [ $# != 1 ]; then
	echo "need benchmark name"
	exit 1
fi
benchmark=$1
PF1=0
PF2=0

cpu_bind() { 
	echo "bind cpu!!"
	dom0_cpu1=0
	dom0_cpu2=2
	dom0_cpu3=3
	dom0_cpu4=4
	domu_cpu=1
	xl vcpu-set 0 4
	xl vcpu-pin 0 0 $dom0_cpu1
	xl vcpu-pin 0 1 $dom0_cpu2
	xl vcpu-pin 0 2 $dom0_cpu3
	xl vcpu-pin 0 3 $dom0_cpu4
	xl vcpu-pin 1 0 $domu_cpu
	echo "------------"
}

init() { 
	echo "init!!"
	xl dm -c >> /dev/null
	$XC_DIR/xc -r
	$XC_DIR/xc -s 0 > tmp.txt
	PF1=`cat tmp.txt | grep "page fault" | awk -F ':' '{print $2}'`
	echo "------------"
}

open_monitor() { 
	echo "open monitor!!"
	#stop and clear
	`$PMU_DIR/a.out 1 $domu_cpu 2 1`
	#start pmu
	`$PMU_DIR/a.out 1 $domu_cpu 1 0`
	# open monitor
	`$PMU_DIR/a.out 2 1 1 0`

	xl dm -c >> /dev/null
	echo "------------"
}

stop_monitor() { 
	echo "close monitor!!"
	# close monitor
	`$PMU_DIR/a.out 2 1 0 0`
	`$PMU_DIR/a.out 1 $domu_cpu 2 1`
	$XC_DIR/xc -s 1 > tmp.txt
	cat tmp.txt
	PF2=`cat tmp.txt | grep "page fault" | awk -F ':' '{print $2}'`
	PFN=$(($PF2-$PF1))
	echo "page fault count:$PFN"
	echo "------------"
}

run_spec() { 
	echo "run spec!!"
	rm 148* -f
	./time_aet.sh $benchmark &
	time_pid=$!
	#ssh 172.17.11.53 "/root/houfang/test_pmu/wssfake/wss_fake $array $round"
	ssh 172.17.11.53 "/root/houfang/benchmark_script/$benchmark.sh"
	kill $time_pid
	echo "------------"
	tmp="hi"
	while [ -n "$tmp" ]
	do
		echo $tmp
		echo "wait aet calculate end"
		tmp=`ps -A | awk '{print $4}'  | grep -w xc`
		sleep 10
	done
	./draw_pic.sh $benchmark
	rm -rf ./temp/$benchmark
	if [ ! -d "./temp/$benchmark" ] ; then
		mkdir -p ./temp/$benchmark
	fi
	mv 148* ./temp/$benchmark
	python test.py $benchmark
}

read_pmu() { 
	echo "read pmu counter!!"
	#xl dm | tail -n 12
	mem1=`xl dm | tail -n 16 | grep "MEM_UOP_RETIRED_ALL1" | awk -F ':' '{print $2}' | awk -F ',' '{print $1}'`
	mem2=`xl dm | tail -n 16 | grep "MEM_UOP_RETIRED_ALL2" | awk -F ':' '{print $2}' | awk -F ',' '{print $1}'`
	mem=$(($mem1+$mem2))
	dtlb1=`xl dm | tail -n 16 | grep "DTLB_LOAD_MISS_COMPLETE" | awk -F ':' '{print $2}' | awk -F ',' '{print $1}'`
	dtlb2=`xl dm | tail -n 16 | grep "DTLB_STORE_MISS_COMPLETE" | awk -F ':' '{print $2}' | awk -F ',' '{print $1}'`
	dtlb=$(($dtlb1+$dtlb2))
	echo "mem1:$mem1 mem2:$mem2 mem:$mem"
	echo "dtlb1:$dtlb1 dtlb2:$dtlb2 dtlb:$dtlb"
	echo "------------"
}

aet() { 
	echo "cal aet!!"
	$XC_DIR/xc -c $mem
	xl dm -c >> /dev/null
	$XC_DIR/xc -s 2
	if [ ! -d "./data/$benchmark" ] ; then
		mkdir ./data/$benchmark
	fi
	if [ ! -d "./graph/$benchmark" ] ; then
		mkdir ./graph/$benchmark
	fi
	mv aet_hist.txt ./data/$benchmark 
	mv miss_curve.txt ./data/$benchmark
	python graph.py -b $benchmark
	echo "------------"
}

end() { 
	rm -f tmp.txt
}

cpu_bind
init
open_monitor
run_spec
stop_monitor
read_pmu
#aet
end
