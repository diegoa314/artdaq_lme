art_make( BASENAME_ONLY
  LIBRARY_NAME artdaq_DAQdata
  EXCLUDE GenericFragmentSimulator_generator.cc
  LIB_LIBRARIES
  ${ARTDAQ-CORE_DATA}
  ${ARTDAQ-CORE_GENERATORS}
  artdaq-utilities_Plugins
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  ${MESSAGEFACILITY_LIBS}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  )

simple_plugin(GenericFragmentSimulator "generator"
  artdaq_DAQdata
  artdaq-core_Data
  ${ARTDAQ-CORE_GENERATORS}
  ${CETLIB_EXCEPT}
  )

install_headers()
install_source()
