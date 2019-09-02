# This function will generate the FHiCL code used to control the
# BoardReaderMain application by configuring its
# artdaq::BoardReaderCore object
  
def generateBoardReaderMain( generatorCode, destinations_fhicl,dataDir, routingCode,  withGanglia = 0, withMsgFacility = 0, withGraphite = 0)

  brConfig = String.new( "\
  daq: {
  fragment_receiver: {
	mpi_sync_interval: 50

	%{generator_code}

	destinations: {
	  %{destinations_fhicl}
	}

	routing_table_config: {
	    %{routing_code}
	}
  }

  metrics: {
	brFile: {
	  metricPluginType: \"file\"
	  level: 3
	  fileName: \"%{datadir}/boardreader/br_%UID%_metrics.log\"
	  uniquify: true
	}
	%{ganglia_metric} ganglia: {
	%{ganglia_metric}   metricPluginType: \"ganglia\"
	%{ganglia_metric}   level: %{ganglia_level}
	%{ganglia_metric}   reporting_interval: 15.0
	%{ganglia_metric} 
	%{ganglia_metric}   configFile: \"/etc/ganglia/gmond.conf\"
	%{ganglia_metric}   group: \"ARTDAQ\"
	%{ganglia_metric} }
	%{mf_metric} msgfac: {
	%{mf_metric}    level: %{mf_level}
	%{mf_metric}    metricPluginType: \"msgFacility\"
	%{mf_metric}    output_message_application_name: \"ARTDAQ Metric\"
	%{mf_metric}    output_message_severity: 0 
	%{mf_metric} }
	%{graphite_metric} graphite: {
	%{graphite_metric}   level: %{graphite_level}
	%{graphite_metric}   metricPluginType: \"graphite\"
	%{graphite_metric}   host: \"localhost\"
	%{graphite_metric}   port: 20030
	%{graphite_metric}   namespace: \"artdaq.\"
	%{graphite_metric} }
  }
}"
)
  
  brConfig.gsub!(/\%\{generator_code\}/, String(generatorCode))
  brConfig.gsub!(/\%\{routing_code\}/, String(routingCode))
  brConfig.gsub!(/\%\{destinations_fhicl\}/, destinations_fhicl)
  
  brConfig.gsub!(/\%\{datadir\}/, dataDir)
  if Integer(withGanglia) > 0
	brConfig.gsub!(/\%\{ganglia_metric\}/, "")
	brConfig.gsub!(/\%\{ganglia_level\}/, String(withGanglia))
  else
	brConfig.gsub!(/\%\{ganglia_metric\}/, "#")
  end
  if Integer(withMsgFacility) > 0
	brConfig.gsub!(/\%\{mf_metric\}/, "")
	brConfig.gsub!(/\%\{mf_level\}/, String(withMsgFacility))
  else
	brConfig.gsub!(/\%\{mf_metric\}/, "#")
  end
  if Integer(withGraphite) > 0
	brConfig.gsub!(/\%\{graphite_metric\}/, "")
	brConfig.gsub!(/\%\{graphite_level\}/, String(withGraphite))
  else
	brConfig.gsub!(/\%\{graphite_metric\}/, "#")
  end
 
  return brConfig
end
