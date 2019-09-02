# ======================================================================
#  artdaq main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/artdaq/ups/setup_for_development
#  cmake [-DCMAKE_INSTALL_PREFIX=/install/path]
#        -Dqualifier:STRING=$CETPKG_QUAL
#        -DCMAKE_BUILD_TYPE=$CETPKG_TYPE
#        $CETPKG_SOURCE
#  make
#  make test
#  make install
#  make package
# ======================================================================

# use cmake 2.8 or later
cmake_minimum_required (VERSION 2.8)

project(artdaq)
# cetbuildtools contains our cmake modules
SET ( CETBUILDTOOLS_VERSION $ENV{CETBUILDTOOLS_VERSION} )
IF (NOT CETBUILDTOOLS_VERSION)
	MESSAGE (FATAL_ERROR "ERROR: setup cetbuildtools to get the cmake modules")
ENDIF()

set(CMAKE_MODULE_PATH
  $ENV{ART_DIR}/Modules
  $ENV{CANVAS_DIR}/Modules
	$ENV{CETBUILDTOOLS_DIR}/Modules
			  $ENV{ARTDAQ_UTILITIES_DIR}/Modules
	${CMAKE_MODULE_PATH})

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS VIGILANT
  WERROR
  NO_UNDEFINED
  EXTRA_FLAGS -pedantic -DRTI_UNIX -DRTI_LINUX -DRTI_64BIT -m64
)

string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC )
if( ${BTYPE_UC} MATCHES "DEBUG" )
	if(DEFINED ENV{USE_ASAN})
		if($ENV{USE_ASAN} GREATER 0)
			message("!!! Address Sanitizer Activated!!!")
			cet_add_compiler_flags(-fsanitize=address -lasan -Wno-unused-command-line-argument)
		endif()
	endif()
	if(DEFINED ENV{USE_TSAN})
		if($ENV{USE_TSAN} GREATER 0)
			message("!!! Thread Sanitizer Activated!!!")
			cet_add_compiler_flags(-fsanitize=thread -ltsan -Wno-unused-command-line-argument)
		endif()
	endif()
endif()

cet_report_compiler_flags()

# JCF, 7/8/15 - not sure this is actually doing anything...
SET( CMAKE_EXE_LINKER_FLAGS_${BTYPE_UC}  " -m64 -static-libgcc")

# these are minimum required versions, not the actual product versions
find_ups_boost(v1_53_0)
find_ups_product (cetbuildtools v4_09_02)
find_ups_product( TRACE v3_03_00 )
find_ups_product( art v1_08_10 )
find_ups_product( canvas v1_00_00 )
find_ups_product( canvas_root_io v1_00_00 )
find_ups_product( artdaq_core v1_03_01 )
find_ups_product( artdaq_utilities v1_00_00)
find_ups_product( cetlib v1_08_00 )
find_ups_product( cetpkgsupport v1_08_03 )
find_ups_product( clhep v2_2_0_3 )
find_ups_product( fhiclcpp v3_04_00 )
find_ups_product( messagefacility v1_14_01 )
find_ups_product( sqlite v3_08_05_00 )
#cet_find_library(SQLITE3 NAMES sqlite3_ups PATHS ENV SQLITE_LIB NO_DEFAULT_PATH)
find_ups_product( tbb v4_1 )
find_ups_product( xmlrpc_c v1_25_18 )
find_ups_root( v5_34_20 )

# Include at the top level to make sure include directory is visible
# everywhere.
include(ProcessSmc)

string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" ART_VERSION_LIST "$ENV{ART_VERSION}")
list(GET ART_VERSION_LIST 0 ART_MAJOR_VERSION)
list(GET ART_VERSION_LIST 1 ART_MINOR_VERSION)
list(GET ART_VERSION_LIST 2 ART_PATCH_VERSION)

# string(LENGTH ${ART_MINOR_VERSION} TMP_LEN)
# if (${TMP_LEN} GREATER 1)
#     string(SUBSTRING ${ART_MINOR_VERSION} 0 1 TMP_STRING)
#     if (${TMP_STRING} EQUAL "0")
#         string(SUBSTRING ${ART_MINOR_VERSION} 1 ${TMP_LEN} ART_MINOR_VERSION)
#     endif()
# endif()

# string(LENGTH ${ART_PATCH_VERSION} TMP_LEN)
# if (${TMP_LEN} GREATER 1)
#     string(SUBSTRING ${ART_PATCH_VERSION} 0 1 TMP_STRING)
#     if (${TMP_STRING} EQUAL "0")
#         string(SUBSTRING ${ART_PATCH_VERSION} 1 ${TMP_LEN} ART_PATCH_VERSION)
#     endif()
# endif()

# message("-DART_MAJOR_VERSION=${ART_MAJOR_VERSION} -DART_MINOR_VERSION=${ART_MINOR_VERSION} -DART_PATCH_VERSION=${ART_PATCH_VERSION}")
# add_definitions(-DART_MAJOR_VERSION=${ART_MAJOR_VERSION} -DART_MINOR_VERSION=${ART_MINOR_VERSION} -DART_PATCH_VERSION=${ART_PATCH_VERSION})
message("-DART_HEX_VERSION=0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION}")
set(ART_HEX_VERSION 0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION})
add_definitions(-DART_HEX_VERSION=0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION})


string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" MESSAGEFACILITY_VERSION_LIST "$ENV{MESSAGEFACILITY_VERSION}")
list(GET MESSAGEFACILITY_VERSION_LIST 0 MESSAGEFACILITY_MAJOR_VERSION)
list(GET MESSAGEFACILITY_VERSION_LIST 1 MESSAGEFACILITY_MINOR_VERSION)
list(GET MESSAGEFACILITY_VERSION_LIST 2 MESSAGEFACILITY_PATCH_VERSION)

message("-DMESSAGEFACILITY_HEX_VERSION=0x${MESSAGEFACILITY_MAJOR_VERSION}${MESSAGEFACILITY_MINOR_VERSION}${MESSAGEFACILITY_PATCH_VERSION}")
add_definitions(-DMESSAGEFACILITY_HEX_VERSION=0x${MESSAGEFACILITY_MAJOR_VERSION}${MESSAGEFACILITY_MINOR_VERSION}${MESSAGEFACILITY_PATCH_VERSION})

# as of art v2_11_00, messagefacility has a single library
check_ups_version(messagefacility $ENV{MESSAGEFACILITY_VERSION} v2_02_00 PRODUCT_MATCHES_VAR ONE_MF_LIBRARY)
if(ONE_MF_LIBRARY)
    set( MESSAGEFACILITY_LIBS MF_MessageLogger)
else()
    set( MESSAGEFACILITY_LIBS MF_MessageLogger MF_Utilities)
endif()

find_library( XMLRPC_SERVER_ABYSSPP NAMES xmlrpc_server_abyss++ PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_SERVER_ABYSS NAMES xmlrpc_server_abyss PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_SERVERPP NAMES xmlrpc_server++ PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_SERVER NAMES xmlrpc_server PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_ABYSS NAMES xmlrpc_abyss PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_CLIENTPP NAMES xmlrpc_client++ PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPCPP NAMES xmlrpc++ PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC NAMES xmlrpc PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_UTIL NAMES xmlrpc_util PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_UTILPP NAMES xmlrpc_util++ PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
# XMLRPC_UTILPP is new to v1_39_07:
if(NOT XMLRPC_UTILPP)
	set(XMLRPC_UTILPP "")
endif()
find_library( XMLRPC_XMLPARSE NAMES xmlrpc_xmlparse PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
find_library( XMLRPC_XMLTOK NAMES xmlrpc_xmltok PATHS $ENV{XMLRPC_C_LIBRARY} NO_DEFAULT_PATH )
#find_library( TRACE trace PATHS $ENV{TRACE_LIB} )

# Debug streamer.
string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC)
if(BTYPE_UC STREQUAL DEBUG)
  add_definitions(-DDEBUGME)
endif()


if (DEFINED ENV{NDDSHOME})
   set( USING_RTIDDS TRUE )
   message("\"NDDSHOME\" environment variable defined; will build RTI-DDS code")
else()
   set( USING_RTIDDS FALSE )
   message("\"NDDSHOME\" environment variable not defined; will ignore RTI-DDS code")
endif()

# macros for artdaq_dictionary and simple_plugin
include(ArtDictionary)
include(ArtMake)
include(BuildPlugins)

# testing
add_subdirectory(test)

# source
add_subdirectory(artdaq)

# Prototype stuff (kept around for tests)
add_subdirectory(proto)

# Useful tools and scripts.
add_subdirectory(tools)

# doc - Documentation
add_subdirectory(doc)

# ups - table and config files
add_subdirectory(ups)

# packaging utility
include(UseCPack)
