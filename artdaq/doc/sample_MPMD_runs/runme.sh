
# setup mvapich2 v1_7_0

do_a_run()
{
  echo "---------- running ${1} ------------"
  mpirun_rsh -rsh -config ${1}_conf_file.txt -hostfile ${1}_host_file.txt
  echo "-- contents of the ${1}_conf_file --"
  cat ${1}_conf_file.txt
  echo "-- contents of the ${1}_host_file --"
  cat ${1}_host_file.txt
  echo
}

do_a_run "r11"
do_a_run "r12"
do_a_run "r22a"
do_a_run "r22b"
do_a_run "r21"
do_a_run "r21u"

# mpirun_rsh -rsh -config r11_conf_file.txt -hostfile r11_host_file.txt
# mpirun_rsh -rsh -config r12_conf_file.txt -hostfile r12_host_file.txt
# mpirun_rsh -rsh -config r22_conf_file.txt -hostfile r22_host_file.txt
# mpirun_rsh -rsh -config r21_conf_file.txt -hostfile r21_host_file.txt

