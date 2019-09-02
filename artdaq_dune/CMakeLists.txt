# ======================================================================
#  artdaq-dune main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/artdaq-demo/ups/setup_for_development
#  buildtool
# ======================================================================


# use cmake 2.8 or later
cmake_minimum_required (VERSION 2.8)


project(artdaq-dune)
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
 EXTRA_FLAGS -pedantic
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

# these are minimum required versions, not the actual product versions
find_ups_product( cetbuildtools v3_03_00 )
find_ups_product( TRACE v3_03_00 )
find_ups_product( artdaq v0_04_02 )
find_ups_product( artdaq_utilities v1_00_00)
find_ups_product( artdaq_core v1_00_00 )
find_ups_product( artdaq_core_dune v0_00_01 )
find_ups_product( art v1_06_00 )
  find_ups_product( canvas v1_00_00 )
find_ups_product( fhiclcpp v2_16_00 )
find_ups_product( messagefacility v1_10_00 )
#find_ups_product( cpp0x v1_03_00 )
find_ups_product( cetlib v1_03_00 )
find_ups_root( v5_34_00 )
find_ups_boost( v1_50_0 filesystem
                        program_options
                        unit_test_framework
 )

string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" ART_VERSION_LIST "$ENV{ART_VERSION}")
list(GET ART_VERSION_LIST 0 ART_MAJOR_VERSION)
list(GET ART_VERSION_LIST 1 ART_MINOR_VERSION)
list(GET ART_VERSION_LIST 2 ART_PATCH_VERSION)

message("-DART_HEX_VERSION=0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION}")
set(ART_HEX_VERSION 0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION})
add_definitions(-DART_HEX_VERSION=0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION})

string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" MESSAGEFACILITY_VERSION_LIST "$ENV{MESSAGEFACILITY_VERSION}")
list(GET MESSAGEFACILITY_VERSION_LIST 0 MESSAGEFACILITY_MAJOR_VERSION)
list(GET MESSAGEFACILITY_VERSION_LIST 1 MESSAGEFACILITY_MINOR_VERSION)
list(GET MESSAGEFACILITY_VERSION_LIST 2 MESSAGEFACILITY_PATCH_VERSION)

message("-DMESSAGEFACILITY_HEX_VERSION=0x${MESSAGEFACILITY_MAJOR_VERSION}${MESSAGEFACILITY_MINOR_VERSION}${MESSAGEFACILITY_PATCH_VERSION}")
add_definitions(-DMESSAGEFACILITY_HEX_VERSION=0x${MESSAGEFACILITY_MAJOR_VERSION}${MESSAGEFACILITY_MINOR_VERSION}${MESSAGEFACILITY_PATCH_VERSION})

string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" CANVAS_VERSION_LIST "$ENV{CANVAS_VERSION}")
list(GET CANVAS_VERSION_LIST 0 CANVAS_MAJOR_VERSION)
list(GET CANVAS_VERSION_LIST 1 CANVAS_MINOR_VERSION)
list(GET CANVAS_VERSION_LIST 2 CANVAS_PATCH_VERSION)

message("-DCANVAS_HEX_VERSION=0x${CANVAS_MAJOR_VERSION}${CANVAS_MINOR_VERSION}${CANVAS_PATCH_VERSION}")
add_definitions(-DCANVAS_HEX_VERSION=0x${CANVAS_MAJOR_VERSION}${CANVAS_MINOR_VERSION}${CANVAS_PATCH_VERSION})

# as of art v2_11_00, messagefacility has a single library
check_ups_version(messagefacility $ENV{MESSAGEFACILITY_VERSION} v2_02_00 PRODUCT_MATCHES_VAR ONE_MF_LIBRARY)
if(ONE_MF_LIBRARY)
    set( MESSAGEFACILITY_LIBS MF_MessageLogger)
else()
    set( MESSAGEFACILITY_LIBS MF_MessageLogger MF_Utilities)
endif()

# macros for art_dictionary and simple_plugin
include(ArtDictionary)
include(ArtMake)
include(BuildPlugins)
include(CetTest)

# source
add_subdirectory(artdaq-dune)

# testing
add_subdirectory(test)

# tools
add_subdirectory(tools)

# doc - Documentation
add_subdirectory(doc)

# ups - table and config files
add_subdirectory(ups)

# packaging utility
include(UseCPack)
