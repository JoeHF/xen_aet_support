benchmark=$1
all_file=`ls 149*miss*.txt`
for file in ${all_file[@]}
do
	#echo $file
	python draw_miss_curve.py -n $file -b $benchmark 
done

all_file=`ls 149*aet_hist.txt`
for file in ${all_file[@]}
do
	#echo $file
	python draw_aet_hist.py -n $file -b $benchmark 
done

all_file=`ls 149*mc*.txt`
for file in ${all_file[@]}
do
	#echo $file
	python draw_lru_miss_curve.py -n $file -b $benchmark 
done

all_file=`ls 149*lru_hist.txt`
for file in ${all_file[@]}
do
	#echo $file
	python draw_lru_hist.py -n $file -b $benchmark 
done
