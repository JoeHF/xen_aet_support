benchmark=$1
name="./temp/$benchmark/tmp.txt"
ls $name
rm $name -f
python cmp_wss.py $benchmark
python test.py $benchmark

