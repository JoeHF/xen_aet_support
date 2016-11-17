#!/bin/sh
xl dm -c >> /dev/null
array=$1
round=$2
domu_cpu=1
dom0_cpu=0
xl vcpu-set 0 1
xl vcpu-pin 0 0 $dom0_cpu
xl vcpu-set 1 1
xl vcpu-pin 1 0 $domu_cpu
XC_DIR=../control
PMU_DIR=./
./a.out 2 1 1 0 # open monitor
xl dm -c
echo "------------------------"
./a.out 2 2 2 0
xl dm -c
echo "------------------------"
$XC_DIR/xc
echo "------------------------"
#stop and clear
`$PMU_DIR/a.out 1 $domu_cpu 2 1`
#start pmu
`$PMU_DIR/a.out 1 $domu_cpu 1 0`
ssh 172.17.11.53 "/root/houfang/test_pmu/wssfake/wss_fake $array $round"
#sleep 27
#stop
`$PMU_DIR/a.out 1 $domu_cpu 2 1`
echo "------------------------"
$XC_DIR/xc
echo "------------------------"
xl dm -c | tail -n 16 
./a.out 2 1 0 0 #close monitor
xl dm -c
echo "------------------------"
