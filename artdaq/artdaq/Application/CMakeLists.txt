process_smc(SMC_LIB_SOURCE Commandable.sm)

include_directories("${CMAKE_BINARY_DIR}")

art_make_library( LIBRARY_NAME artdaq_Application
  SOURCE
  ${SMC_LIB_SOURCE}
  Commandable.cc
  CommandableFragmentGenerator
  makeCommandableFragmentGenerator
  BoardReaderApp.cc
  BoardReaderCore.cc
  EventBuilderCore.cc
  EventBuilderApp.cc
  DataLoggerApp.cc
  DataLoggerCore.cc
  DataReceiverCore.cc
  DispatcherApp.cc
  DispatcherCore.cc
  RoutingMasterApp.cc
  RoutingMasterCore.cc
  StatisticsHelper.cc
  LIBRARIES
  artdaq_DAQrate
  artdaq-core_Data
  artdaq-core_Utilities
  artdaq_Application_Routing
  ${MF_MESSAGELOGGER}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  canvas
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${FHICLCPP}
  )

simple_plugin(CompositeDriver "generator" artdaq_DAQdata artdaq_Application)

install_headers()
install_source()
install_fhicl()

add_subdirectory(Routing)
