#!/bin/sh
xl dm -c
array=$1
round=$2
domu_cpu=1
dom0_cpu=0
xl vcpu-set 0 1
xl vcpu-pin 0 0 $dom0_cpu
xl vcpu-set 1 1
xl vcpu-pin 1 0 $domu_cpu
PMU_DIR=./
#stop and clear
`$PMU_DIR/a.out $domu_cpu 2 1`
#start pmu
`$PMU_DIR/a.out $domu_cpu 1 0`
ssh 172.17.11.53 "/root/houfang/test_pmu/wssfake/wss_fake $array $round"
#sleep 27
#stop
`$PMU_DIR/a.out $domu_cpu 2 1`
xl dm -c
