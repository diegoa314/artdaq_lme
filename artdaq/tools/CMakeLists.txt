# Make sure tests have correct environment settings.
include(CetTest)
# If you explicitly include CetTest in a subdirectory, you will need to
# re-initialize the test environment.
cet_test_env("FHICL_FILE_PATH=.")

install_scripts( installMessageViewer.sh installEpicsPlugin.sh installGangliaPlugin.sh disk_stats.sh)

art_make_exec( periodic_cmd_stats )

art_make_exec(StateResponder
  LIBRARIES
  artdaq_Application
  artdaq_ExternalComms
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

art_make_exec(genToArt
  LIBRARIES
  artdaq_Application
  artdaq_DAQrate
  artdaq_DAQdata_GenericFragmentSimulator_generator
  artdaq-core_Utilities
  artdaq-core_Generators
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  )

  cet_script(setupMpichPlugin.sh)

# Make sure tests have correct environment settings.
include(CetTest)
# If you explicitly include CetTest in a subdirectory, you will need to
# re-initialize the test environment.
cet_test_env("FHICL_FILE_PATH=.")

cet_test(genToArt_t HANDBUILT
  TEST_EXEC genToArt
  TEST_ARGS -c genToArt_t.fcl
  DATAFILES fcl/genToArt_t.fcl
)

cet_test(genToArt_outToBinaryFileOutput_t HANDBUILT
  TEST_EXEC genToArt
  TEST_ARGS -c genToArt_outToBinaryFileOutput_t.fcl
  DATAFILES fcl/genToArt_outToBinaryFileOutput_t.fcl
)

#cet_test(genToArt_outToBinaryNetOutput_t HANDBUILT
#  TEST_EXEC mpirun
#  TEST_ARGS genToArt -c genToArt_outToBinaryNetOutput_t.fcl :  s_r_handles 1
#  DATAFILES fcl/genToArt_outToBinaryNetOutput_t.fcl)

install_source()
