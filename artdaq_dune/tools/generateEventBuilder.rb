
def generateEventBuilder( totalFragments, fullEventBuffSizeWords, verbose, sources_fhicl, dataDir,tokenConfig, sendRequests = 0, withGanglia = 0, withMsgFacility = 0, withGraphite = 0)

ebConfig = String.new( "\
daq: {
  event_builder: {
	expected_fragments_per_event: %{total_fragments}
	max_event_size_bytes: %{size_bytes}
	use_art: true
	print_event_store_stats: true
	buffer_count: 20
	send_init_fragments: false
	verbose: %{verbose}
	send_requests: %{requests_enabled}
	
	routing_token_config: {
	%{token_config}
	}

	sources: {
		%{sources_fhicl}
	}
  }
  metrics: {
	evbFile: {
	  metricPluginType: \"file\"
	  level: 3
	  fileName: \"%{datadir}/eventbuilder/evb_%UID%_metrics.log\"
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
} "
)

  ebConfig.gsub!(/\%\{total_fragments\}/, String(totalFragments))
  ebConfig.gsub!(/\%\{size_bytes\}/, String(fullEventBuffSizeWords * 8))
  ebConfig.gsub!(/\%\{verbose\}/, String(verbose))
  ebConfig.gsub!(/\%\{sources_fhicl\}/, sources_fhicl)
  ebConfig.gsub!(/\%\{token_config\}/, tokenConfig)

  ebConfig.gsub!(/\%\{datadir\}/, dataDir)

  if Integer(withGanglia) > 0
	ebConfig.gsub!(/\%\{ganglia_metric\}/, "")
	ebConfig.gsub!(/\%\{ganglia_level\}/, String(withGanglia))
  else
	ebConfig.gsub!(/\%\{ganglia_metric\}/, "#")
  end
  if Integer(withMsgFacility) > 0
	ebConfig.gsub!(/\%\{mf_metric\}/, "")
	ebConfig.gsub!(/\%\{mf_level\}/, String(withMsgFacility))
  else
	ebConfig.gsub!(/\%\{mf_metric\}/, "#")
  end
  if Integer(withGraphite) > 0
	ebConfig.gsub!(/\%\{graphite_metric\}/, "")
	ebConfig.gsub!(/\%\{graphite_level\}/, String(withGraphite))
  else
	ebConfig.gsub!(/\%\{graphite_metric\}/, "#")
  end
  if Integer(sendRequests) > 0
	ebConfig.gsub!(/\%\{requests_enabled\}/, "true")
  else
	ebConfig.gsub!(/\%\{requests_enabled\}/, "false")
  end

  return ebConfig

end

