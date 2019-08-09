include(artdaq_doxygen)

create_doxygen_documentation(EXCLUDE
	artdaq/ArtModules/RootDAQOutput/RootDAQOutFile.h
	artdaq/ArtModules/RootDAQOutput/RootDAQOutFile.cc
	artdaq/ArtModules/RootDAQOutput/RootDAQOut_module.cc
	artdaq/ArtModules/RootDAQOutput/detail/KeptProvenance.h
	artdaq/ArtModules/RootDAQOutput/detail/KeptProvenance.cc
)
