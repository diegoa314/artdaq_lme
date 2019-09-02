
simple_plugin(FragmentSniffer "module"
  artdaq_DAQdata
  art_Framework_Core
  art_Framework_Principal
  art_Persistency_Provenance
  MF_MessageLogger
  fhiclcpp
  ${CETLIB}
  ${CETLIB_EXCEPT}
  )

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

if(${art_not_in_ups})
else(${art_not_in_ups})
 set(artVersionLib $ENV{ART_LIB}/libart_Version.so)
endif(${art_not_in_ups})

if( ${ART_HEX_VERSION} LESS 0x20906 )
cet_test(shared_memory_reader_t USE_BOOST_UNIT
  LIBRARIES
  artdaq_ArtModules
  artdaq_DAQrate
  artdaq_DAQdata
  artdaq-core_Data_dict
  art_Framework_Core
  art_Framework_Principal
  art_Persistency_Provenance
 # canvas_Persistency_Common_dict
 # canvas_Persistency_WrappedStdDictionaries_dict
  art_Utilities
  ${artVersionLib}
  )
  endif()

cet_test(daq_flow_t
  LIBRARIES
  art_Framework_Art
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  artdaq_DAQdata_GenericFragmentSimulator_generator
  artdaq_DAQrate
  artdaq_DAQdata
  MF_MessageLogger
  fhiclcpp
  cetlib
  TEST_ARGS -c daq_flow_t.fcl
  DATAFILES daq_flow_t.fcl
  )

  cet_test(reconfigure_t
   LIBRARIES
  art_Framework_Art
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  artdaq_DAQdata_GenericFragmentSimulator_generator
  artdaq_DAQrate
  artdaq_DAQdata
  MF_MessageLogger
  fhiclcpp
  cetlib
  TEST_ARGS -c daq_flow_t.fcl
  DATAFILES daq_flow_t.fcl
  )

# Wait for an input for a second, then exit.
cet_test(read_files_t HANDBUILT
  TEST_EXEC art_ut
  TEST_ARGS -- -c read_files_t.fcl
  DATAFILES read_files_t.fcl
  )

# Generate several fragments and verify they made it into the store.
cet_test(driver_t HANDBUILT
  TEST_EXEC artdaqDriver
  TEST_ARGS -c driver_t.fcl
  DATAFILES driver_t.fcl
  )

