cet_script(setupDemoEnvironment.sh DemoControl.rb 
startSystem.sh manageSystem.sh
startXxYSystem.sh manageXxYSystem.sh
start2x2System.sh manage2x2System.sh 
start2x2x2System.sh manage2x2x2System.sh
start2x2x2x2System.sh manage2x2x2x2System.sh
start2x2x4System.sh manage2x2x4System.sh
CreateNewFragmentGenerator.rb  
generateEventBuilderMain.rb generateEventBuilder.rb
generateWFViewer.rb demo_utilities.rb
generateFragmentReceiver.rb
generateAggregatorMain.rb generateAggregator.rb
generateRoutingMasterMain.rb
generateBoardReaderMain.rb RunDriver.rb
makeConfigs.rb
multi_udp_send_artdaq.py 
run_demo.sh xt_cmd.sh)


#art_make_exec(demo_driver
#  LIBRARIES
#  artdaq_Application
#  artdaq_DAQrate
#  artdaq-core_Utilities
#  artdaq-core_Core
#  art_Framework_Art
#  ${FHICLCPP}
#  ${Boost_PROGRAM_OPTIONS_LIBRARY}
#  ${MPI_C_LIBRARIES}
#  ${CETLIB}
#  )

art_make_exec(read_fhicl SOURCE readfhicl.cc
  LIBRARIES
  ${FHICLCPP}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${CETLIB}
  )  

add_subdirectory(snippets)

# Is this necessary?
#install_source()
install_fhicl(SUBDIRS fcl)

