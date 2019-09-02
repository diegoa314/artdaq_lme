# This function will generate the FHiCL code used to control the
# AggregatorMain application by configuring its
# artdaq::AggregatorCore object

require File.join( File.dirname(__FILE__), 'generateAggregator' )

def generateAggregatorMain(dataDir, bunchSize, is_data_logger, onmonEnable,
                           diskWritingEnable, demoPrescale, agIndex, totalDLs, totalDPs, fragSizeWords,
						   dl_sources_fhicl, dl_destinations_fhicl, dp_sources_fhicl,
                           xmlrpcClientList, filePropertiesFhicl,
						    subrunSizeThreshold, subrunDuration, subrunEventCount,
							fclWFViewer, onmonEventPrescale, 
                           onmon_modules, onmonFileEnable, onmonFileName, tokenConfig,
                           withGanglia, withMsgFacility, withGraphite)

agConfig = String.new( "\
services: {
  scheduler: {
    errorOnFailureToPut: false
  }
  NetMonTransportServiceInterface: {
    service_provider: NetMonTransportService
    %{rootmpi_output}broadcast_sends: true
    %{rootmpi_output}nonblocking_sends: true
	%{rootmpi_output}destinations: {	
	%{rootmpi_output}  %{destinations_fhicl}
    %{rootmpi_output}}
  }

  #SimpleMemoryCheck: { }
}

%{aggregator_code}

source: {
  module_type: NetMonInput
}
outputs: {
  %{rootmpi_output}rootNetOutput: {
  %{rootmpi_output}  module_type: RootNetOutput
  %{rootmpi_output}  #SelectEvents: { SelectEvents: [ pmod2,pmod3 ] }
  %{rootmpi_output}}

  %{root_output}normalOutput: {
  %{root_output}  module_type: RootOutput
  %{root_output}  fileName: \"%{output_file}\"
  %{root_output}  %{fileproperties}
  %{root_output}  fastCloning: false
  %{root_output}  compressionLevel: 3
  %{root_output}}

  %{root_output2}normalOutputMod2: {
  %{root_output2}  module_type: RootOutput
  %{root_output2}  fileName: \"%{output_file_mod2}\"
  %{root_output2}  SelectEvents: { SelectEvents: [ pmod2 ] }
  %{root_output2}   %{fileproperties}
  %{root_output2}  fastCloning: false
  %{root_output2}  compressionLevel: 3
  %{root_output2}}

  %{root_output2}normalOutputMod3: {
  %{root_output2}  module_type: RootOutput
  %{root_output2}  fileName: \"%{output_file_mod3}\"
  %{root_output2}  SelectEvents: { SelectEvents: [ pmod3 ] }
  %{root_output2}   %{fileproperties}
  %{root_output2}  fastCloning: false
  %{root_output2}  compressionLevel: 3
  %{root_output2}}

}
physics: {
  analyzers: {
%{phys_anal_onmon_cfg}

   checkintegrity: {
     module_type: CheckIntegrity
     raw_data_label: daq
     frag_type: TOY1
   }

  }

  producers: {

     BuildInfo:
     {
       module_type: ArtdaqDemoBuildInfo
       instance_name: ArtdaqDemo
     }
   }

  filters: {

    prescaleMod2: {
       module_type: NthEvent
       nth: 2
    }

    prescaleMod3: {
       module_type: NthEvent
       nth: 3
    }
  }

  p2: [ BuildInfo ]
  pmod2: [ prescaleMod2 ]
  pmod3: [ prescaleMod3 ]

  %{enable_onmon}a1: %{onmon_modules}
  #a2: [ checkintegrity ]

  %{root_output}my_output_modules: [ normalOutput ]
  %{root_output2}my_output_modules: [ normalOutputMod2, normalOutputMod3 ]
  %{rootmpi_output}my_mpi_output_modules: [ rootNetOutput ]
}
process_name: DAQ%{daqproccode}"
)

  queueDepth, queueTimeout = -999, -999

  if is_data_logger > 0
    if totalDPs > 1
      onmonEnable = 0
    end
    queueDepth = 20
    queueTimeout = 5
    agType = "data_logger"
	agCode = "DL"
  else
    diskWritingEnable = 0
    queueDepth = 20
    queueTimeout = 1
    agType = "online_monitor"
	agCode = "DISP"
  end

  aggregator_code = generateAggregator( bunchSize, fragSizeWords, dl_sources_fhicl, dp_sources_fhicl,
                                        xmlrpcClientList, subrunSizeThreshold, subrunDuration, subrunEventCount, 
										queueDepth, queueTimeout, onmonEventPrescale,
										agType, dataDir, tokenConfig,
										withGanglia, withMsgFacility, withGraphite )
  agConfig.gsub!(/\%\{aggregator_code\}/, aggregator_code)

  puts "Initial aggregator " + String(agIndex) + " disk writing setting = " +
  String(diskWritingEnable)
  # Do the substitutions in the aggregator configuration given the options
  # that were passed in from the command line.  Assure that files written out
  # by each AG are unique by including a timestamp in the file name.
  currentTime = Time.now
  fileName = "artdaqdemo_"
  fileName += "r%06r_sr%02s_%to_%#"
  need_index = totalDLs > 1
  if need_index
    fileName += "_"
    fileName += String(agIndex)
  end
  fileName += ".root"
  outputFile = File.join(dataDir, fileName)

  agConfig.gsub!(/\%\{output_file\}/, outputFile)
  agConfig.gsub!(/\%\{fileproperties\}/, filePropertiesFhicl)
  agConfig.gsub!(/\%\{output_file_mod2\}/, outputFile.sub(".root", "_mod2.root"))
  agConfig.gsub!(/\%\{output_file_mod3\}/, outputFile.sub(".root", "_mod3.root"))

  agConfig.gsub!(/\%\{onmon_modules\}/, String(onmon_modules))

  puts "agIndex = %d, totalDLs = %d, totalDPs = %d, onmonEnable = %d" % [agIndex, totalDLs, totalDPs, onmonEnable]

  puts "Final aggregator " + String(agIndex) + " disk writing setting = " +
  String(diskWritingEnable)
  if Integer(diskWritingEnable) != 0
    if Integer(demoPrescale) != 0
      agConfig.gsub!(/\%\{root_output\}/, "#")
      agConfig.gsub!(/\%\{root_output2\}/,"")
    else
      agConfig.gsub!(/\%\{root_output\}/, "")
      agConfig.gsub!(/\%\{root_output2\}/,"#")
    end
  else
    agConfig.gsub!(/\%\{root_output\}/, "#")
    agConfig.gsub!(/\%\{root_output2\}/,"#")
  end

  if is_data_logger > 0 && totalDPs > 0
	agConfig.gsub!(/\%\{rootmpi_output\}/,"")
	agConfig.gsub!(/\%\{destinations_fhicl\}/, dl_destinations_fhicl)
  else
	agConfig.gsub!(/\%\{rootmpi_output\}/,"#")
  end

  agConfig.gsub!(/\%\{daqproccode\}/,agCode)

  if Integer(onmonEnable) != 0
    agConfig.gsub!(/\%\{phys_anal_onmon_cfg\}/, fclWFViewer )
    agConfig.gsub!(/\%\{enable_onmon\}/, "")
    if Integer(onmonFileEnable) != 0
      agConfig.gsub!(/\%\{onmon_file_enable\}/,"")
      agConfig.gsub!(/\%\{onmon_fileName\}/,"fileName: \"" + onmonFileName + "\"")
    else 
      agConfig.gsub!(/\%\{onmon_file_enable\}/,"#")
    end
  else
    agConfig.gsub!(/\%\{phys_anal_onmon_cfg\}/, "")
    agConfig.gsub!(/\%\{enable_onmon\}/, "#")
  end

  return agConfig  
end
