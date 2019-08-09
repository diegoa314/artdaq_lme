
rm -f all_conf.txt
touch all_conf.txt

for n in r_conf*
do
	cat $n >> all_conf.txt
	
done

# rm -f all_perf.txt
# touch all_perf.txt

# for n in r_perf*
# do
	# tail -1 $n >> all_perf.txt
# done
