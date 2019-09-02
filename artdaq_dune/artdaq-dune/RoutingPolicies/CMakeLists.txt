simple_plugin("MisbehaviorTest" "policy"
    artdaq_Application_Routing
    artdaq_DAQdata
    ${FHICLCPP}
    ${MF_MESSAGELOGGER}
  ${Boost_SYSTEM_LIBRARY}
    cetlib_except
    cetlib
    )

simple_plugin(NthEvent "policy"
artdaq_Application_Routing
artdaq_DAQdata
${FHICLCPP}
${MF_MESSAGELOGGER}
  ${Boost_SYSTEM_LIBRARY}
cetlib_except
cetlib
)
install_headers()
install_fhicl(SUBDIRS fcl)
install_source()
