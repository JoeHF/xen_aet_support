if [ $# != 1 ] ; then
	echo "need benchmark name"
	exit 1
fi
benchmark=$1
scp tcount100.txt 172.17.2.106:/houfang/xen-4.5.1/aet_tool/control/pin_data/$benchmark
scp rtd.txt 172.17.2.106:/houfang/xen-4.5.1/aet_tool/control/pin_data/$benchmark
scp result.txt 172.17.2.106:/houfang/xen-4.5.1/aet_tool/control/pin_data/$benchmark
