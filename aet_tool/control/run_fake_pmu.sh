#!/bin/sh
tmp_file=tmp.txt
xl dm -c >> /dev/null
array=$1
round=$2
domu_cpu=1
dom0_cpu=0
xl vcpu-set 0 1
xl vcpu-pin 0 0 $dom0_cpu
#xl vcpu-set 1 1
xl vcpu-pin 1 0 $domu_cpu
XC_DIR=./
PMU_DIR=./
./a.out 2 1 1 0 # open monitor
xl dm -c
echo "------------------------"
$XC_DIR/xc -r
$XC_DIR/xc -s 0 > tmp.txt
cat tmp.txt
PF1=`cat tmp.txt | grep "page fault" | awk -F ':' '{print $2}'`
echo "------------------------"
#stop and clear
`$PMU_DIR/a.out 1 $domu_cpu 2 1`
#start pmu
`$PMU_DIR/a.out 1 $domu_cpu 1 0`
ssh 172.17.11.53 "/root/houfang/test_pmu/wssfake/wss_fake $array $round"
#sleep 1
#stop
`$PMU_DIR/a.out 1 $domu_cpu 2 1`
echo "------------------------"
$XC_DIR/xc -s 1 > tmp.txt
cat tmp.txt
PF2=`cat tmp.txt | grep "page fault" | awk -F ':' '{print $2}'`
echo "------------------------"
xl dm | tail -n 12
mem1=`xl dm | tail -n 16 | grep "MEM_UOP_RETIRED_ALL1" | awk -F ':' '{print $2}' | awk -F ',' '{print $1}'`
mem2=`xl dm | tail -n 16 | grep "MEM_UOP_RETIRED_ALL2" | awk -F ':' '{print $2}' | awk -F ',' '{print $1}'`
mem=$(($mem1+$mem2))
dtlb1=`xl dm | tail -n 16 | grep "DTLB_LOAD_MISS_COMPLETE" | awk -F ':' '{print $2}' | awk -F ',' '{print $1}'`
dtlb2=`xl dm | tail -n 16 | grep "DTLB_STORE_MISS_COMPLETE" | awk -F ':' '{print $2}' | awk -F ',' '{print $1}'`
dtlb=$(($dtlb1+$dtlb2))
echo "mem1:$mem1 mem2:$mem2 mem:$mem"
echo "dtlb1:$dtlb1 dtlb2:$dtlb2 dtlb:$dtlb"
echo "------------------------"
$XC_DIR/xc -c $mem
./a.out 2 1 0 0 #close monitor
xl dm -c >> /dev/null
echo "------------------------"
$XC_DIR/xc -s 2
python graph.py -m $array -r $round
echo "------------------------"
echo "$PF2 - $PF1"
PFN=$(($PF2-$PF1))
echo $PFN
echo "------------------------"
rm -f tmp.txt
