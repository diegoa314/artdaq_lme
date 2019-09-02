include_directories($ENV{NDDSHOME}/include $ENV{NDDSHOME}/include/ndds )

set( ARCH "x64Linux2.6gcc4.4.5" ) # Needed for RTI-DDS

art_make_library( LIBRARY_NAME artdaq_RTIDDS
  SOURCE
  RTIDDS.cc
  LIBRARIES
  artdaq-core_Data
  ${MF_MESSAGELOGGER}
  ${MF_UTILS}
  $ENV{NDDSHOME}/lib/${ARCH}/libnddscpp.so
  $ENV{NDDSHOME}/lib/${ARCH}/libnddsc.so
  $ENV{NDDSHOME}/lib/${ARCH}/libnddscore.so
)

install_headers()
install_source()
