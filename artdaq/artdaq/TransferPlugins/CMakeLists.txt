
if (${USING_RTIDDS})
include_directories($ENV{NDDSHOME}/include $ENV{NDDSHOME}/include/ndds )
endif()

art_make( BASENAME_ONLY
  LIBRARY_NAME artdaq_TransferPlugins
  NO_PLUGINS
  SUBDIRS detail
  EXCLUDE RTIDDS_transfer.cc Shmem_transfer.cc Multicast_transfer.cc TCPSocket_transfer.cc Autodetect_transfer.cc
  LIB_LIBRARIES
  artdaq-core_Utilities
  artdaq-core_Data
  artdaq_DAQdata
  )


simple_plugin(Shmem "transfer"
  artdaq_TransferPlugins
  artdaq-core_Core
)

simple_plugin(Null "transfer"
  artdaq_TransferPlugins
  )

simple_plugin(Multicast "transfer"
  artdaq_TransferPlugins
  ${Boost_SYSTEM_LIBRARY}
  pthread
)

simple_plugin(TCPSocket "transfer"
  artdaq_TransferPlugins
  ${Boost_THREAD_LIBRARY}
  pthread
)

simple_plugin(Autodetect "transfer"
  artdaq_TransferPlugins
  artdaq_TransferPlugins_Shmem_transfer
  artdaq_TransferPlugins_TCPSocket_transfer
)

if (${USING_RTIDDS})

simple_plugin(RTIDDS "transfer"
  artdaq_TransferPlugins
  artdaq_RTIDDS
)

endif()

install_headers(SUBDIRS detail)
install_fhicl(SUBDIRS fcl)
install_source(SUBDIRS detail)
