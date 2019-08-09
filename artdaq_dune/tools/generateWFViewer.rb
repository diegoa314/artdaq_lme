
# Generate the FHiCL document which configures the demo::WFViewer class

# Note that in the case of the "prescale" and "digital_sum_only"
# parameters, if one or both of these parameters are "nil", the
# function, their values will be taken from a separate file called
# WFViewer.fcl, searched for via "read_fcl"



require File.join( File.dirname(__FILE__), 'demo_utilities' )

def generateWFViewer(fragmentIDList, prescale = nil, digital_sum_only = nil, configDoc = "WFViewer.fcl")

  wfViewerConfig = String.new( "\
    app: {
      module_type: RootApplication
      force_new: true
    }
    wf: {
      module_type: WFViewer
      fragment_ids: %{fragment_ids} " \
      + read_fcl(configDoc) \
      + "    }" )


    # John F., 1/21/14 -- before sending FHiCL configurations to the
    # EventBuilderMain and AggregatorMain processes, construct the
    # strings listing fragment ids and fragment types which will be
    # used by the WFViewer

    # John F., 3/8/14 -- adding a feature whereby the fragments are
    # sorted in ascending order of ID

    fragmentIDListString = "[ "
	
    fragmentIDList.sort.each { |id| fragmentIDListString += " %d," % [ id ] }

    fragmentIDListString[-1] = "]"

  wfViewerConfig.gsub!(/\%\{fragment_ids\}/, String(fragmentIDListString))

if ! prescale.nil?
  wfViewerConfig.gsub!(/.*prescale.*\:.*/, "prescale: %d" % [prescale])
end

if ! digital_sum_only.nil?
  wfViewerConfig.gsub!(/.*digital_sum_only.*\:.*/, "digital_sum_only: %s" % 
                       [String(digital_sum_only) ] )
end

  return wfViewerConfig
end

