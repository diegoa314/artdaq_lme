#!/bin/env ruby


require "erb"
require "open3"

firstport = 5200

system = [
      {"rank" => 0, "tier" =>0, "rpc"=>3, "name" => "BoardReader",  "conf" => ["boardreader.erb.fcl",     {"src" => [0,1], "tgt" => [2,3], "fpe" =>1 , "epb" =>1  }],"host"=> "localhost", },
      {"rank" => 1, "tier" =>0, "rpc"=>3, "name" => "BoardReader",  "conf" => ["boardreader.erb.fcl",     {"src" => [1,2], "tgt" => [2,3], "fpe" =>1 , "epb" =>1  }],"host"=> "localhost", },
  
      {"rank" => 2, "tier" =>1, "rpc"=>4, "name" => "EventBuilder", "conf" => ["eventbuilder_t1.erb.fcl", {"src" => [0,1], "tgt" => [4,5], "fpe" =>1 , "epb" =>1  }],"host"=> "localhost", },
      {"rank" => 3, "tier" =>1, "rpc"=>4, "name" => "EventBuilder", "conf" => ["eventbuilder_t1.erb.fcl", {"src" => [0,1], "tgt" => [4,5], "fpe" =>1 , "epb" =>1  }],"host"=> "localhost", },
  
      {"rank" => 4, "tier" =>2, "rpc"=>4, "name" => "EventBuilder", "conf" => ["eventbuilder_t2.erb.fcl", {"src" => [2,3], "tgt" => [6,7], "fpe" =>1 , "epb" =>1  }],"host"=> "localhost", },
      {"rank" => 5, "tier" =>2, "rpc"=>4, "name" => "EventBuilder", "conf" => ["eventbuilder_t2.erb.fcl", {"src" => [2,3], "tgt" => [6,7], "fpe" =>1 , "epb" =>1  }],"host"=> "localhost", },

      {"rank" => 6, "tier" =>3, "rpc"=>5, "name" => "Aggregator",   "conf" => ["aggregator.erb.fcl",      {"src" => [4,5], "tgt" => []    , "fpe" =>1 , "epb" =>1  }],"host"=> "localhost", },
      {"rank" => 7, "tier" =>3, "rpc"=>5, "name" => "Aggregator",   "conf" => ["aggregator.erb.fcl",      {"src" => [4,5], "tgt" => []    , "fpe" =>1 , "epb" =>1  }],"host"=> "localhost", }
  ]

build_xmlrpc_client_list = lambda{ |s, p| 
  list = "" 
  s.each {|c| list+=(";http://%s:%s/RPC2,%s" % [ c["host"], (p + c["rank"]).to_s, c["rpc"].to_s])} 
  return "\"" + list + "\"" 
}

class ConfigWriter
  def initialize config, xmlrpclist, firstport
    @@firstport = firstport
    @rank = config["rank"]
    @tier = config["tier"]
    basePath = ENV['ARTDAQ_DEMO_DIR']
    if ! ENV['ARTDAQ_DEMO_DIR']
      basePath = ENV['ARTDAQDEMO_REPO']
    end
    @infile = basePath + "/tools/snippets/" + config["conf"][0]
    @outfile = "%s_%s_%s." % [config["name"] , config["host"], (@@firstport + config["rank"]).to_s ]
    @conf = config["conf"][1]
    @xmlrpclist = xmlrpclist
  end

  def write
    templated_fcl = File.new(@outfile+"template.fcl",File::CREAT|File::TRUNC|File::RDWR)        
    templated_fcl.write(ERB.new(File.read(@infile)).result( binding ))
    templated_fcl.close

    expanded_fcl = File.new(@outfile+"fcl",File::CREAT|File::TRUNC|File::RDWR)
    Open3.popen3("readfhicl","-c",@outfile+"template.fcl") { |i, o,e, t| expanded_fcl.write(o.read) }
    expanded_fcl.close
  end
end

#main program
system.each {|cfg| ConfigWriter.new(cfg,build_xmlrpc_client_list.call(system,firstport),firstport).write }    

