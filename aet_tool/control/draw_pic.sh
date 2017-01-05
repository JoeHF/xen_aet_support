benchmark=$1
all_file=`ls 148*miss*.txt`
for file in ${all_file[@]}
do
	echo $file
	python draw_miss_curve.py -n $file -b $benchmark 
done

all_file=`ls 148*aet*.txt`
for file in ${all_file[@]}
do
	echo $file
	python draw_aet_hist.py -n $file -b $benchmark 
done
