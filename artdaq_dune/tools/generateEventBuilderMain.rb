# This function will generate the FHiCL code used to control the
# EventBuilderMain application by configuring its
# artdaq::EventBuilderCore object

require File.join( File.dirname(__FILE__), 'generateEventBuilder' )


def generateEventBuilderMain(ebIndex, totalAGs, dataDir, onmonEnable, diskWritingEnable, totalFragments, fullEventBuffSizeWords, filePropertiesFhicl,
                         fclWFViewer, sources_fhicl, destinations_fhicl, tokenConfig,tableConfig,  sendRequests, withGanglia, withMsgFacility, withGraphite )
  # Do the substitutions in the event builder configuration given the options
  # that were passed in from the command line.  

  ebConfig = String.new( "\

services: {
  scheduler: {
    errorOnFailureToPut: false
  }
  NetMonTransportServiceInterface: {
    service_provider: NetMonTransportService
    #broadcast_sends: true
	destinations: {	
	  %{destinations_fhicl}
    }
	routing_table_config: {
	    %{table_config}
	}
  }

  #SimpleMemoryCheck: { }
}

%{event_builder_code}

outputs: {
  %{rootmpi_output}rootNetOutput: {
  %{rootmpi_output}  module_type: RootNetOutput
  %{rootmpi_output}  #SelectEvents: { SelectEvents: [ pmod2,pmod3 ] }
  %{rootmpi_output}}
  %{root_output}normalOutput: {
  %{root_output}  module_type: RootOutput
  %{root_output}  fileName: \"%{output_file}\"
  %{root_output}  #SelectEvents: { SelectEvents: [ pmod2,pmod3 ] }
  %{root_output}  %{fileProperties}
  %{root_output}  compressionLevel: 3
  %{root_output}  fastCloning: false
 %{root_output}}
}

physics: {
  analyzers: {
%{phys_anal_onmon_cfg}
  }

  producers: {
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

  pmod2: [ prescaleMod2 ]
  pmod3: [ prescaleMod3 ]
   

  %{enable_onmon}a1: [ app, wf ]

  %{rootmpi_output}my_output_modules: [ rootNetOutput ]
  %{root_output}my_output_modules: [ normalOutput ]
}
source: {
  module_type: DemoInput
  waiting_time: 2500000
  resume_after_timeout: true
}
process_name: DAQ" )

verbose = "true"

if Integer(totalAGs) >= 1
  verbose = "false"
end


event_builder_code = generateEventBuilder( totalFragments, fullEventBuffSizeWords, verbose, sources_fhicl,dataDir,tokenConfig, sendRequests, withGanglia, withMsgFacility, withGraphite)

ebConfig.gsub!(/\%\{destinations_fhicl\}/, destinations_fhicl)
ebConfig.gsub!(/\%\{table_config\}/, tableConfig)
ebConfig.gsub!(/\%\{event_builder_code\}/, event_builder_code)

  ebConfig.gsub!(/\%\{fileProperties\}/, filePropertiesFhicl)

if Integer(totalAGs) >= 1
  ebConfig.gsub!(/\%\{rootmpi_output\}/, "")
  ebConfig.gsub!(/\%\{root_output\}/, "#")
  ebConfig.gsub!(/\%\{enable_onmon\}/, "#")
  ebConfig.gsub!(/\%\{phys_anal_onmon_cfg\}/, "")
else
  ebConfig.gsub!(/\%\{rootmpi_output\}/, "#")
  if Integer(diskWritingEnable) != 0
    ebConfig.gsub!(/\%\{root_output\}/, "")
  else
    ebConfig.gsub!(/\%\{root_output\}/, "#")
  end
  if Integer(onmonEnable) != 0
    ebConfig.gsub!(/\%\{phys_anal_onmon_cfg\}/, fclWFViewer )
    ebConfig.gsub!(/\%\{enable_onmon\}/, "")
  else
    ebConfig.gsub!(/\%\{phys_anal_onmon_cfg\}/, "")
    ebConfig.gsub!(/\%\{enable_onmon\}/, "#")
  end
end


currentTime = Time.now
fileName = "artdaqdemo_eb%02d_" % ebIndex
fileName += "r%06r_sr%02s_%to_%#"
fileName += ".root"
outputFile = File.join(dataDir, fileName)
ebConfig.gsub!(/\%\{output_file\}/, outputFile)

return ebConfig

end


