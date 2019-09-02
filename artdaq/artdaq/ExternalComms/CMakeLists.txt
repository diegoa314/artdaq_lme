#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

art_make( BASENAME_ONLY
  LIBRARY_NAME artdaq_ExternalComms
  LIB_LIBRARIES artdaq_Application
  EXCLUDE xmlrpc_commander.cc
  NO_PLUGINS
  artdaq_DAQdata
  ${FHICLCPP}
  ${CETLIB}
)

simple_plugin(xmlrpc "commander"
  artdaq_ExternalComms
  ${FHICLCPP}
  ${XMLRPC_SERVER_ABYSSPP}
  ${XMLRPC_SERVER_ABYSS}
  ${XMLRPC_SERVERPP}
  ${XMLRPC_CLIENT}
  ${XMLRPC_CLIENTPP}
  ${XMLRPC_SERVER}
  ${XMLRPC_ABYSS}
  ${XMLRPCPP}
  ${XMLRPC}
  ${XMLRPC_UTIL}
  ${XMLRPC_UTILPP}
  ${XMLRPC_XMLPARSE}
  ${XMLRPC_XMLTOK}
)

install_headers()
install_source()
