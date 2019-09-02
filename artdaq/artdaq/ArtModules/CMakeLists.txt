add_subdirectory(RootDAQOutput)

art_make( BASENAME_ONLY
  LIBRARY_NAME artdaq_ArtModules
  SUBDIRS detail
  NO_PLUGINS
  ${ART_PERSISTENCY_PROVENANCE}
  ${CANVAS_PERSISTENCY_PROVENANCE}
  LIB_LIBRARIES
  artdaq-core_Data
  artdaq-core_Data_dict
  artdaq_DAQrate
  artdaq_DAQdata
  artdaq_Application
  artdaq_ExternalComms
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_IO_Root
  art_Framework_IO_RootVersion
  art_Framework_IO
  art_Framework_Principal
  artdaq_ArtModules_NetMonTransportService_service
  art_Persistency_Provenance
  #  art_Persistency_RootDB
  canvas
  MF_MessageLogger
  ${SQLITE3}
  ${ROOT_CINTEX}
  ${ROOT_RIO}
  ${ROOT_TREE}
  ${ROOT_CORE}
  ${ROOT_NET}
  ${ROOT_REFLEX}
  ${Boost_DATE_TIME_LIBRARY}
  )

simple_plugin(RawInput "source"
  artdaq_ArtModules
  artdaq-core_Data
  artdaq-core_Data_dict
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_Principal
  art_Persistency_Provenance
  art_Utilities
  MF_MessageLogger
  rt
)

simple_plugin(TransferOutput "module"
  artdaq_TransferPlugins
  artdaq_ArtModules
  art_Framework_IO_Root
  MF_MessageLogger
  fhiclcpp
  cetlib
  ${CETLIB_EXCEPT}
  ${ROOT_RIO}
  ${TBB}
)

simple_plugin(TransferInput "source"
  artdaq-core_Data
  artdaq-core_Data_dict
  artdaq_TransferPlugins
  artdaq_ArtModules
  ${TBB}
)

simple_plugin(RandomDelayFilter "module" artdaq_ArtModules)

simple_plugin(EventDump "module"
  artdaq-core_Data
  MF_MessageLogger
  ${CETLIB_EXCEPT})

simple_plugin(MissingDataCheck "module"
  artdaq-core_Data
  artdaq-core_Data_dict
  MF_MessageLogger
  art_Framework_Services_Registry
  art_Framework_Services_Optional
  art_Framework_Services_Optional_TFileService_service
  ${ROOT_BASIC_LIB_LIST}
  ${CETLIB_EXCEPT}
)


simple_plugin(NetMonInput "source"
  artdaq-core_Data
  artdaq-core_Data_dict
  artdaq_ArtModules_NetMonTransportService_service
  artdaq_ArtModules
  artdaq-core_Utilities
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_Principal
  art_Persistency_Provenance
  art_Utilities
  ${MESSAGEFACILITY_LIBS}
  fhiclcpp
  cetlib
  ${CETLIB_EXCEPT}
  ${TBB}
)

simple_plugin(RootNetOutput "module"
  artdaq_ArtModules
  artdaq-core_Data_dict
  art_Framework_IO_Root
  MF_MessageLogger
  fhiclcpp
  cetlib
  ${CETLIB_EXCEPT}
  ${ROOT_RIO}
  ${TBB}
)

simple_plugin(BinaryNetOutput "module"
  artdaq_ArtModules
  art_Framework_Services_Registry
  art_Utilities
  MF_MessageLogger
  fhiclcpp
  cetlib
  ${CETLIB_EXCEPT}
  ${ROOT_RIO}
)

simple_plugin(BinaryFileOutput "module"
  artdaq_ArtModules
  art_Framework_IO
  art_Framework_Services_Registry
  art_Utilities
  MF_MessageLogger
  fhiclcpp
  cetlib
  ${CETLIB_EXCEPT}
  ${ROOT_RIO}
)

simple_plugin(NetMonTransportService "service"
  artdaq-core_Data
  artdaq-core_Data_dict
  artdaq_DAQrate
  art_Framework_Services_Registry
  MF_MessageLogger
  fhiclcpp
  ${ROOT_RIO}
  ${ROOT_NET}
  ${ROOT_CORE}
)

simple_plugin(ArtdaqBuildInfo "module"
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_BuildInfo
  ${MF_MESSAGELOGGER}
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
)

simple_plugin(PrintBuildInfo "module"
  MF_MessageLogger
  fhiclcpp
  ${CETLIB}
  ${CETLIB_EXCEPT}
  )


install_headers(SUBDIRS detail)
install_source(SUBDIRS detail)
install_fhicl(SUBDIRS fcl)
