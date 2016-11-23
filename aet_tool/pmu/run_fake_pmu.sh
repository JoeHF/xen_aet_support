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
XC_DIR=../control
PMU_DIR=./
./a.out 2 1 1 2 # open monitor
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
xl dm -c | tail -n 16 
./a.out 2 1 0 0 #close monitor
xl dm -c
echo "------------------------"
echo "$PF2 - $PF1"
PFN=$(($PF2-$PF1))
echo $PFN
echo "------------------------"
rm -f tmp.txt
