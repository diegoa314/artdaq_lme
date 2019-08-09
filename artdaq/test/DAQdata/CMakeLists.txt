
cet_test(GenericFragmentSimulator_t USE_BOOST_UNIT
  LIBRARIES artdaq_DAQdata_GenericFragmentSimulator_generator
  )
  
cet_test(tracemf_t HANDBUILT
  TEST_EXEC tracemf
  TEST_ARGS -csutdl 100000
  )