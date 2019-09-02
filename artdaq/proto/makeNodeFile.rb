#!/usr/bin/env ruby

# this script must be used to generate an mpirun hostfile
# to run builder.  The configuration generated will match the
# needs of builder to make the correct rank assignments on 
# each of the nodes.
# NOTE: the function num_source_nodes below and other calculations
# must match the algorithms in Config.cc of the builder application.

def num_source_nodes(total_nodes,sources_per,detectors_per)
  total_nodes.to_f / (sources_per.to_f/detectors_per.to_f + 1.0)
end

def generate_dot(run,blist,dlist)
end

def makeNodeFile(file_name,detectors_per,sources_per,sinks_per,run_num,node_count)
  
  nodes = File.read(file_name).split
  total_nodes=nodes.size
  save_filename = "hostfile_#{run_num}.txt"
  save_filenamem = "hostfilem_#{run_num}.txt"
  save_file = File.new(save_filename,"w")
  save_filem = File.new(save_filenamem,"w")

  # we do not do anything in this case
  #if total_nodes==1
  #  return 0
  #end

  if node_count > 0
    nodes = nodes[0,total_nodes]
  end

  # detectors must be on separate node
  # sources and sinks are both on the other nodes
  # tot=7,src_per=2,det_per=3 -> 4 builders
  # tot=7,src_per=1,det_per=3 -> 5 builders
  # tot=6,src_per=2,det_per=3 -> 3 builders
  # tot=5,src_per=2,det_per=3 -> 3 builders

  builder_nodes = num_source_nodes(total_nodes,sources_per,detectors_per).to_i

  return 0 if builder_nodes==0

  sources=builder_nodes * sources_per
  sinks=builder_nodes * sinks_per

  needed_det_nodes = (sources.to_f / detectors_per).ceil
  return 0 if needed_det_nodes==0

  det_last_slot = sources-((needed_det_nodes-1)*detectors_per)
  detectors=(needed_det_nodes-1)*detectors_per + det_last_slot
  detector_nodes=needed_det_nodes

  # detectors are first, sources second, and sinks third in rank

  if detectors < sources
    #puts "Bad configuration: total detectors must equal total sources"
    #puts " detectors = #{detectors}, sources = #{sources}"
    return 0
  end

  detector_list=nodes[0,detector_nodes]
  builder_list=nodes[detector_nodes,builder_nodes]

  # p detector_list
  # p builder_list
  # puts "S: bn=#{builder_nodes} dn=#{detector_nodes}"
  # puts "D=#{detectors} S=#{sources} P=#{sinks}"
  # puts "F: needed_det_nodes=#{needed_det_nodes} last_slot=#{det_last_slot}"
  # puts "sources=#{sources}"
  # puts "needed=#{needed_det_nodes} det_last=#{det_last_slot}"
  # puts "----"

  full_list=[]

  detector_list[0,needed_det_nodes-1].each do |n|
    detectors_per.to_i.times {|i| full_list << "#{n}" }
    save_file.puts "#{n} slots=#{detectors_per.to_i}"
  end
  
  if det_last_slot>0
    last_name = detector_list[needed_det_nodes-1]
    det_last_slot.to_i.times {|i| full_list << "#{last_name}" }
    save_file.puts "#{detector_list[needed_det_nodes-1]} slots=#{det_last_slot.to_i}"
  end

  builder_list.each do |n|
    sources_per.to_i.times {|i| full_list << "#{n}" }
    save_file.puts "#{n} slots=#{sources_per.to_i}" 
  end
  builder_list.each do |n|
    sinks_per.to_i.times {|i| full_list << "#{n}" }
    save_file.puts "#{n} slots=#{sinks_per.to_i}"
  end

  # full_list.each {|i| puts "#{i}" }
  d_list=detectors.to_i.times.collect {|i| "D#{i}" }
  s_list=sources.to_i.times.collect {|i| "S#{i}" }
  p_list=sinks.to_i.times.collect {|i| "P#{i}" }
  workers= d_list + s_list + p_list

  clusters=full_list.zip(workers).inject(Hash.new {|h,k| h[k]=[]}){|h,v| h[v[0]]<<v[1];h}

  full_list.each {|i| save_filem.puts "#{i}" }

  # p full_list
  # p workers
  # p clusters
  # puts "full_list sz=#{full_list.size} workers.size=#{workers.size}"
  # 

  dotfile=File.new("graph_#{run_num}.dot","w")
  dotfile.puts "digraph G {"
  clusters.each_pair do |k,v|
    dotfile.puts "subgraph cluster_#{k} {"
    v.each {|n| dotfile.puts "#{n} [shape=Msquare];" }
    dotfile.puts "}"
  end
  d_list.zip(s_list).each {|i| dotfile.puts "#{i[0]} -> #{i[1]}" }
  s_list.product(p_list).each {|i| dotfile.puts "#{i[0]} -> #{i[1]}" }
  dotfile.puts "}"
  dotfile.close

  return (detectors+sources+sinks)
end

# main processing
if __FILE__ == $PROGRAM_NAME

  if ARGV.size < 5
    puts "arguments: NodesFile DetectorsPerNode SourcesPerNode SinksPerNode RunNum [TotalNodes]"
    exit 1
  end

  file_name=ARGV[0] # input name
  detectors_per=ARGV[1].to_f
  sources_per=ARGV[2].to_f
  sinks_per=ARGV[3].to_f
  run_num=ARGV[4].to_i
  total_nodes=0 # means number in file
  
  # override allowed for testing
  if ARGV.size > 5
    total_nodes=ARGV[5].to_i
  end
  
  tot=makeNodeFile(file_name,detectors_per,sources_per,sinks_per,run_num,total_nodes)

  puts "#{tot.to_i}"
end
