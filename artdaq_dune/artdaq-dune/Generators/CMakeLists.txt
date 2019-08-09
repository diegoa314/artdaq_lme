
simple_plugin(ToySimulator "generator"
  artdaq-demo_Generators_ToyHardwareInterface
  artdaq-core-demo_Overlays
  artdaq_Application
  artdaq_DAQdata
  artdaq-core_Utilities
  artdaq-core_Data
  artdaq-utilities_Plugins
  canvas
  ${FHICLCPP}
  ${MESSAGEFACILITY_LIBS}
  ${CETLIB}
${CETLIB_EXCEPT}
  )

simple_plugin(AsciiSimulator "generator"
  artdaq-core-demo_Overlays
  artdaq_Application
  artdaq_DAQdata
  artdaq-core_Utilities
  artdaq-core_Data
  ${Boost_SYSTEM_LIBRARY}
  ${FHICLCPP}
  ${MF_MESSAGELOGGER}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  )

simple_plugin(UDPReceiver "generator"
    artdaq-core-demo_Overlays
    artdaq_Application
    artdaq_DAQdata
    artdaq-core_Utilities
    artdaq-core_Data
  ${Boost_SYSTEM_LIBRARY}
  canvas
    ${FHICLCPP}
  ${MESSAGEFACILITY_LIBS}
    ${CETLIB}
 ${CETLIB_EXCEPT}
    )

add_subdirectory(ToyHardwareInterface)
