#!/usr/bin/env ruby

# RUN=1 EVENTS=10 BUFFERS=10 NODES=2 EVENT_SIZE=100000
# DETS_PER=3 SRCS_PER=3 SINKS_PER=2
#
# Here is what I think the test ranges are for each that are interesting.
#
# DETS_PER [1,3]
# SRCS_PER [1,4]
# SINKS_PER [1,4]
# EVENT_SIZE {50K,100K,200K,400K,800K,2M,4M,8M}
# NODES {4,8,16,32}

dryrun=false
dryrun=true if ARGV.size > 0 && ARGV[0]=='dryrun'
dryrun=true if ARGV.size > 1 && ARGV[1]=='dryrun'
mvapich=false
mvapich=true if ARGV.size > 0 && ARGV[0]=='mvapich'

puts "mvapich true" if mvapich

I_SIZE=0
I_DETS=1
I_SRCS=2
I_SINKS=3

pbs_nodefile = ENV['PBS_NODEFILE']

if pbs_nodefile == nil
  pbs_nodefile="./tmp_nodefile"
  File.open(pbs_nodefile, 'w') do |f|
    8.times {|i| f.puts "host#{i}" }
  end
end

KB=1024
MB=KB*KB

NUM_EVENTS=20000

if mvapich
 num_procs="-np"
 launcher="mpirun_rsh_1"
 host_base="hostfilem"
 scale=0.4
else
 num_procs="-n"
 launcher="mpirun"
 host_base="hostfile"
 scale=1.0
end

require './short_runs'
short_runs = get_short_runs("short_runs.txt",scale)

dets_per=(1..3).to_a
srcs_per=(1..4).to_a
sinks_per=(1..4).to_a
event_sizes=[50*KB,100*KB,200*KB,400*KB,800*KB,2*MB,4*MB,8*MB]
buffers=5

# will need to be fixed for now - the number of nodes in the NODESFILE
# nodes=[4,8,16,32]
nodes=File.open(pbs_nodefile,"r") { |f| f.count }

comb=event_sizes.product(dets_per,srcs_per,sinks_per)

require './makeNodeFile'
require './cat'
require 'fileutils'

# p short_runs

comb.inject(1) do |run,test|

  # puts "#{run} #{test}"
  if short_runs.has_key?(run)
 
    # prepare a hostfile for the given configuration
    np=makeNodeFile(pbs_nodefile,test[I_DETS],test[I_SRCS],test[I_SINKS],run,0)
    hostfile="#{host_base}_#{run}.txt"

    # prepare arguments and run
    args="#{nodes} #{short_runs[run]} #{test[I_DETS]} #{test[I_SRCS]} #{test[I_SINKS]} #{test[I_SIZE]} #{buffers} #{run}"
    cmd="#{launcher} -hostfile #{hostfile} #{num_procs} #{np} ./builder #{args}"

    if dryrun == true
      puts "#{cmd}"
      rc=0
    else
      puts "#{cmd}" if np > 0
      # rc=`#{cmd}` if np > 0
    end

    # round up the config and performance files next
    pfiles=Dir.glob("perf_*#{run}_*")
    cfiles=Dir.glob("config_*#{run}_*")
    catb(pfiles, "r_perf_#{run}.txt") unless pfiles.empty?
    cat(cfiles, "r_conf_#{run}.txt") unless cfiles.empty?
    FileUtils.rm(pfiles) unless pfiles.empty?
    FileUtils.rm(cfiles) unless cfiles.empty?
  end
  run+1
end
