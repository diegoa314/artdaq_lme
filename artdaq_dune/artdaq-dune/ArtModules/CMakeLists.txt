art_make( BASENAME_ONLY
 LIBRARY_NAME artdaq-dune_ArtModules
  NO_PLUGINS
  DICT_DICTIONARIES
  ${ART_PERSISTENCY_PROVENANCE}
  ${CANVAS_PERSISTENCY_PROVENANCE}
 LIB_LIBRARIES
 artdaq-core-dune_Overlays
 artdaq_ArtModules
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_Principal
  art_Persistency_Provenance
  canvas
  MF_MessageLogger
  ${ROOT_CINTEX}
  ${ROOT_RIO}
  ${ROOT_CORE}
  ${ROOT_NET}
  ${ROOT_REFLEX}
  cetlib
  ${CETLIB_EXCEPT}
)

#art_make(NO_PLUGINS # WAnt different link lists for modues
#  LIB_LIBRARIES artdaq-core-demo_Overlays
#)

# foreach (mod NthEvent )
#   simple_plugin(${mod} "module" artdaq-demo_ArtModules)
# endforeach()

simple_plugin(ArtdaqDemoBuildInfo "module"
  artdaq_DAQdata
  artdaq_BuildInfo
  artdaq-core_BuildInfo
  artdaq-dune_BuildInfo
  artdaq-core-dune_BuildInfo
  ${MF_MESSAGELOGGER}
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
)

simple_plugin(ToyDumpPrueba "module"
             artdaq-core_Data
			  artdaq-core-dune_Overlays
 ${CETLIB_EXCEPT})
simple_plugin(Graficador "module"
             artdaq-core_Data
			  artdaq-core-dune_Overlays
			  MF_MessageLogger	
			  ${ROOT_GPAD}
			  ${ROOT_HIST}
			  ${ROOT_GUI}
			  ${ROOT_RIO}	
 ${CETLIB_EXCEPT})


simple_plugin(NthEvent "module"
  ${CETLIB_EXCEPT})
simple_plugin(ToyDump "module"
             artdaq-core_Data
			  artdaq-core-dune_Overlays
 ${CETLIB_EXCEPT})
simple_plugin(ASCIIDump "module"
              artdaq-core_Data
			  artdaq-core-dune_Overlays
  ${CETLIB_EXCEPT})

simple_plugin(RootApplication "module"
  ${CETLIB_EXCEPT})
simple_plugin(WFViewer "module"
              artdaq-core_Data
			  artdaq-core-dune_Overlays
			  MF_MessageLogger
			  ${ROOT_GPAD}
			  ${ROOT_HIST}
			  ${ROOT_GUI}
			  ${ROOT_RIO}
  ${CETLIB_EXCEPT})

simple_plugin(CheckIntegrity "module"
              artdaq-core_Data
			  artdaq-core-dune_Overlays
  ${MESSAGEFACILITY_LIBS}
  ${CETLIB_EXCEPT}
)

simple_plugin(DemoInput "source"
 artdaq-core-dune_Overlays
  artdaq-core_Core
  artdaq-core_Utilities
  rt
  artdaq_DAQdata
  artdaq-utilities_Plugins
  artdaq_ArtModules
  art_Framework_Core
  art_Framework_IO_Sources
  art_Framework_Principal
  art_Persistency_Provenance
  art_Utilities
  MF_MessageLogger
  ${CETLIB_EXCEPT}
)

simple_plugin(EventReporterOutput "module"
  artdaq_ArtModules
  art_Framework_Services_Registry
  art_Utilities
  MF_MessageLogger
  fhiclcpp
  cetlib
  ${CETLIB_EXCEPT}
  ${ROOT_RIO}
)
install_headers(SUBDIRS detail)
install_source(SUBDIRS detail)
install_fhicl(SUBDIRS fcl)
