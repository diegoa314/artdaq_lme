
def generateAggregator(bunchSize, fragSizeWords, dl_sources_fhicl, dp_sources_fhicl,
                       xmlrpcClientList,
					   subrunSizeThreshold, subrunDuration, subrunEventCount, 
					   queueDepth, queueTimeout, onmonEventPrescale,
                       agType, dataDir, tokenConfig,
					   withGanglia = 0, withMsgFacility = 0, withGraphite = 0)

agConfig = String.new( "\
daq: {
  aggregator: {
    expected_fragments_per_event: %{bunch_size}
	max_fragment_size_bytes: %{size_bytes}
    print_event_store_stats: true
    buffer_count: %{queue_depth}
    event_queue_wait_time: %{queue_timeout}
    onmon_event_prescale: %{onmon_event_prescale}
    xmlrpc_client_list: \"%{xmlrpc_client_list}\"
    subrun_size_MB: %{subrun_size}
    subrun_duration: %{subrun_duration}
    subrun_event_count: %{subrun_event_count}
    %{ag_type_param_name}: true

	routing_token_config: {
		%{token_config}
	}

	auto_suppression_enabled: false
	sources: {
		%{sources_fhicl}
	}
  }

  metrics: {
    aggFile: {
      metricPluginType: \"file\"
      level: 3
      fileName: \"%{datadir}/aggregator/agg_%UID%_metrics.log\"
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
}" )

  agConfig.gsub!(/\%\{size_words\}/, String(fragSizeWords))
  agConfig.gsub!(/\%\{size_bytes\}/, String(fragSizeWords * 8))
  agConfig.gsub!(/\%\{bunch_size\}/, String(bunchSize))  
  agConfig.gsub!(/\%\{queue_depth\}/, String(queueDepth))  
  agConfig.gsub!(/\%\{queue_timeout\}/, String(queueTimeout))  
  agConfig.gsub!(/\%\{onmon_event_prescale\}/, String(onmonEventPrescale))
  agConfig.gsub!(/\%\{xmlrpc_client_list\}/, String(xmlrpcClientList))
  agConfig.gsub!(/\%\{subrun_size\}/, String(subrunSizeThreshold))
  agConfig.gsub!(/\%\{subrun_duration\}/, String(subrunDuration))
  agConfig.gsub!(/\%\{subrun_event_count\}/, String(subrunEventCount))
  agConfig.gsub!(/\%\{token_config\}/, tokenConfig)
  if agType == "online_monitor"
    #agConfig.gsub!(/\%\{ag_type_param_name\}/, "is_online_monitor")
    agConfig.gsub!(/\%\{ag_type_param_name\}/, "is_dispatcher")
    agConfig.gsub!(/\%\{sources_fhicl\}/, dp_sources_fhicl)
  else
    agConfig.gsub!(/\%\{ag_type_param_name\}/, "is_data_logger")
    agConfig.gsub!(/\%\{sources_fhicl\}/, dl_sources_fhicl)
  end
  

  agConfig.gsub!(/\%\{datadir\}/, dataDir)
  if Integer(withGanglia) > 0
    agConfig.gsub!(/\%\{ganglia_metric\}/, "")
    agConfig.gsub!(/\%\{ganglia_level\}/, String(withGanglia))
  else
    agConfig.gsub!(/\%\{ganglia_metric\}/, "#")
  end
  if Integer(withMsgFacility) > 0
    agConfig.gsub!(/\%\{mf_metric\}/, "")
    agConfig.gsub!(/\%\{mf_level\}/, String(withMsgFacility))
  else
    agConfig.gsub!(/\%\{mf_metric\}/, "#")
  end
  if Integer(withGraphite) > 0
    agConfig.gsub!(/\%\{graphite_metric\}/, "")
    agConfig.gsub!(/\%\{graphite_level\}/, String(withGraphite))
  else
    agConfig.gsub!(/\%\{graphite_metric\}/, "#")
  end

  return agConfig
end
