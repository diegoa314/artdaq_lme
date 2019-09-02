
art_make( BASENAME_ONLY
    LIBRARY_NAME artdaq_ArtModules_RootDAQOutput
  NO_PLUGINS
  ${ART_PERSISTENCY_PROVENANCE}
  ${CANVAS_PERSISTENCY_PROVENANCE}
  LIB_LIBRARIES
  artdaq-core_Data
  artdaq_DAQrate
  artdaq_DAQdata
  artdaq_Application
  artdaq_ExternalComms
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_IO_Root
  art_Framework_IO_RootVersion
  art_Framework_IO
  art_Framework_IO_detail
  art_Framework_IO_Root_detail_sources
  art_Framework_IO_Root_RootDB
  art_Framework_IO_ProductMix
  art_Framework_Principal
  artdaq_ArtModules_NetMonTransportService_service
  art_Persistency_Provenance
#  art_Persistency_RootDB
  ${CANVAS_LIB_LIST}
  canvas_root_io
  MF_MessageLogger
  ${SQLITE3}
  ${ROOT_CINTEX}
  ${ROOT_RIO}
  ${ROOT_TREE}
  ${ROOT_CORE}
  ${ROOT_NET}
  ${ROOT_REFLEX}
  ${Boost_DATE_TIME_LIBRARY}
  ${TBB}
  )

simple_plugin(RootDAQOut "module"
  artdaq_ArtModules_NetMonTransportService_service
  artdaq_ArtModules
  artdaq_ArtModules_RootDAQOutput
  art_Framework_Services_Registry
  art_Framework_IO_Root
  art_Framework_IO_RootVersion
  art_Framework_IO_Root_detail_sources  #   art_root_DictionaryChecker
  art_Framework_IO_ProductMix
#  art_Persistency_RootDB
  art_Utilities
  MF_MessageLogger
  fhiclcpp
  cetlib
  ${Boost_DATE_TIME_LIBRARY}
  ${CETLIB_EXCEPT}
  ${SQLITE3}
  canvas_root_io
  ${ROOT_CORE}
  ${ROOT_RIO}
  ${ROOT_TREE}
)


