benchmark=$1
all_file=`ls 148*.txt`
for file in ${all_file[@]}
do
	echo $file
	python draw_miss_curve.py -n $file -b $benchmark 
done
