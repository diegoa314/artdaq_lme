# ----------------------------------------------------------------------
# Build this project's executables:

# Nominally a test exec (no MPI) but we want this to be available to other packages.
art_make_exec(NAME artdaqDriver
	SOURCE
	driver.cc
  LIBRARIES
  artdaq_Application # needed for messagefacility
  artdaq_DAQrate
  artdaq_DAQdata
  artdaq-core_Utilities
  artdaq-core_Generators
  ${FHICLCPP}
  ${CETLIB}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

art_make_exec(NAME fhicl_test
	SOURCE
	fhicl_test.cc
	LIBRARIES
	${FHICLCPP}
	${CETLIB}
	${CETLIB_EXCEPT}
	${MF_MESSAGELOGGER}
	${Boost_PROGRAM_OPTIONS_LIBRARY}
	)

  art_make_exec(NAME artdaq
	SOURCE
	artdaq.cc
  LIBRARIES
  artdaq_Application # needed for messagefacility
  artdaq_DAQrate
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_Utilities
  artdaq-core_Generators
  artdaq_ExternalComms
  ${FHICLCPP}
  ${CETLIB}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

  art_make_exec(NAME boardreader
	SOURCE
	boardreader.cc
  LIBRARIES
  artdaq_Application # needed for messagefacility
  artdaq_DAQrate
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_Utilities
  artdaq-core_Generators
  artdaq_ExternalComms
  ${FHICLCPP}
  ${CETLIB}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

  art_make_exec(NAME eventbuilder
	SOURCE
	eventbuilder.cc
  LIBRARIES
  artdaq_Application # needed for messagefacility
  artdaq_DAQrate
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_Utilities
  artdaq-core_Generators
  artdaq_ExternalComms
  ${FHICLCPP}
  ${CETLIB}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

  art_make_exec(NAME datalogger
	SOURCE
	datalogger.cc
  LIBRARIES
  artdaq_Application # needed for messagefacility
  artdaq_DAQrate
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_Utilities
  artdaq-core_Generators
  artdaq_ExternalComms
  ${FHICLCPP}
  ${CETLIB}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

  art_make_exec(NAME dispatcher
	SOURCE
	dispatcher.cc
  LIBRARIES
  artdaq_Application # needed for messagefacility
  artdaq_DAQrate
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_Utilities
  artdaq-core_Generators
  artdaq_ExternalComms
  ${FHICLCPP}
  ${CETLIB}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

  art_make_exec(NAME requestReceiver
	  SOURCE requestReceiver.cc
	  LIBRARIES
	  artdaq_Application
	  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

  art_make_exec(NAME requestSender
	  SOURCE requestSender.cc
	  LIBRARIES
	  artdaq_Application
	  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

  art_make_exec(NAME routing_master
  SOURCE
  routing_master.cc
  LIBRARIES
  artdaq_Application
  artdaq_BuildInfo
  artdaq_ExternalComms
  ${FHICLCPP}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

art_make_exec(NAME transfer_plugin_sender # NO_INSTALL -- comment out to install
  LIBRARIES
  artdaq_TransferPlugins
  artdaq-core_Data
  ${CETLIB}
  ${FHICLCPP}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  pthread
  )

art_make_exec(NAME transfer_plugin_receiver # NO_INSTALL -- comment out to install
  LIBRARIES
  artdaq_TransferPlugins
  artdaq-core_Data
  ${CETLIB}
  ${FHICLCPP}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  pthread
  )

art_make_exec(NAME tracemf
	SOURCE
	tracemf.cc
	LIBRARIES
	artdaq_DAQdata
	artdaq-core_Utilities
	${Boost_PROGRAM_OPTIONS_LIBRARY}
	)

	art_make_exec(NAME PrintSharedMemory
	SOURCE PrintSharedMemory.cc
	LIBRARIES
	artdaq_DAQrate
	artdaq-core_Core
	${Boost_PROGRAM_OPTIONS_LIBRARY}
	)

install_fhicl(SUBDIRS fcl)
install_source()
