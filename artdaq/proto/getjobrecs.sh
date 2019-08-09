

FNAME="../all_job_recs.txt"

cd big8
rm -f $FNAME
touch $FNAME

for n in r_perf*
do
 echo $n
 gunzip -c $n | grep -E 'jobstart|jobend' >> ${FNAME}
done

cd ..
