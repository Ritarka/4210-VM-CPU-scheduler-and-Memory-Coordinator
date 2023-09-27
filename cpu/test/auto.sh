for i in `seq 1 3`
do
    script vcpu_scheduler${i}.log
    python monitor.py -t runtest${i}.py
    exit
    echo ${i};
done