#exp1
mem=$1
echo $mem
cd ../../
filename="experiment/shadow_page_fault_number/result_$mem.txt"
rm -f $filename
for i in `seq 0 10` 
do
	./run_fake_pmu.sh $mem $i >> $filename
	echo "$i:"
	tail -n 2 $filename
done
cd -
