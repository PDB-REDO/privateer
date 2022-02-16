## Project settings
#
cmake_minimum_required(VERSION 3.15)
project(privateer VERSION 0.5 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

option(BUILD_SHARED_LIBS "Build as a shared library" ON)

# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)
 
# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_LIBDIR}")
 
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

set(PRIVATEER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/privateer)
set(PRIVATEERDATA ${CMAKE_SOURCE_DIR}/data)

option(CCP4 "The location where ccp4 is installed" "")
if(DEFINED CCP4 AND EXISTS ${CCP4})
	set(CLIBD ${CCP4}/lib/data)
elseif(EXISTS $ENV{CCP4})
	set(CCP4 $ENV{CCP4})
	set(CLIBD ${CCP4}/lib/data)
endif()

if(EXISTS "${CCP4}")
	list(PREPEND CMAKE_MODULE_PATH "${CCP4}/Lib")
	list(APPEND CMAKE_PREFIX_PATH ${CCP4})
	if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		set(CMAKE_INSTALL_PREFIX ${CCP4})
	endif()
endif()

if(MSVC)
    # make msvc standards compliant...
    add_compile_options(/permissive-)

	macro(get_WIN32_WINNT version)
		if (WIN32 AND CMAKE_SYSTEM_VERSION)
			set(ver ${CMAKE_SYSTEM_VERSION})
			string(REPLACE "." "" ver ${ver})
			string(REGEX REPLACE "([0-9])" "0\\1" ver ${ver})

			set(${version} "0x${ver}")
		endif()
	endmacro()

	get_WIN32_WINNT(ver)
	add_definitions(-D_WIN32_WINNT=${ver})
endif()

## Find Privateer's dependencies that have been compiled locally. 
#

list(PREPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/dependencies/lib)

find_package(CCP4 REQUIRED ccp4c clipper-core clipper-ccp4 clipper-contrib clipper-minimol clipper-mmdb clipper-cif)
find_package(Python REQUIRED COMPONENTS Development Interpreter)
find_library(MMDB2 NAMES mmdb2 REQUIRED)
find_library(CCP4SRS NAMES ccp4srs REQUIRED)
find_PATH(CCP4SRS_INCLUDE_DIR ccp4srs/ccp4srs_manager.h REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# First create the library

list(APPEND PRIVATEER_LIBRARY_SOURCES
	${PRIVATEER_SOURCE_DIR}/cpp/third-party/gemmi_compilation_unit.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/clipper-glyco.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/clipper-glyco_data.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-blobs.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-composition.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-dbquery.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-cryo_em.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-xray.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-modelling.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-interactions.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-lib.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pybind11.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-ccp4mg.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-restraints.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pyanalysis.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pymodelling.cpp)

list(APPEND PRIVATEER_LIBRARY_HEADERS
	${PRIVATEER_SOURCE_DIR}/cpp/clipper-glyco.h
	${PRIVATEER_SOURCE_DIR}/cpp/clipper-glyco_data.h
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-blobs.h
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-error.h
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-interactions.h
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-json.h
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-modelling.h
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-xray.h
	${PRIVATEER_SOURCE_DIR}/cpp/third-party/sajson.h
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-composition.h
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-cryo_em.h
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-dbquery.h
	${PRIVATEER_SOURCE_DIR}/cpp/privateer-lib.h
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pyanalysis.h
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pymodelling.h
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-restraints.h)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(privateer-lib ${PRIVATEER_LIBRARY_SOURCES} ${PRIVATEER_LIBRARY_HEADERS})

target_include_directories(privateer-lib
	PUBLIC
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include;${CMAKE_SOURCE_DIR}/dependencies/include;${PRIVATEER_SOURCE_DIR}/cpp/pybind11;${PRIVATEER_SOURCE_DIR}/cpp;${CMAKE_SOURCE_DIR}/dependencies/gemmi/include>"
	"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
	${CCP4_INCLUDE_DIRS}
	${Python_INCLUDE_DIRS}
	${CCP4SRS_INCLUDE_DIR}
	PRIVATE
	${PYBIND11_INCLUDE_DIR})

target_link_libraries(privateer-lib ${CCP4_LIBRARIES} ${PYTHON_LIBRARY} ${MMDB2} ${CCP4SRS} Threads::Threads)

set_target_properties (privateer-lib
	PROPERTIES
	OUTPUT_NAME privateer
	POSITION_INDEPENDENT_CODE ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_link_options(privateer-lib PRIVATE -undefined dynamic_lookup)
endif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")

generate_export_header(privateer-lib
	EXPORT_FILE_NAME privateer-lib/privateer-export.hpp)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} )
set(LIBRARY_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR} )
set(SHARE_INSTALL_DIR ${CMAKE_INSTALL_DATADIR}/libprivateer)

# Install rules

install(TARGETS privateer-lib
	EXPORT privateerTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(MSVC AND BUILD_SHARED_LIBS)
	install(
		FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
		DESTINATION ${CMAKE_INSTALL_LIBDIR}
		OPTIONAL)
endif()

install(EXPORT privateerTargets
	FILE "privateerTargets.cmake"
	NAMESPACE privateer::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/privateer
)

install(
	FILES ${PRIVATEER_LIBRARY_HEADERS} "${CMAKE_CURRENT_BINARY_DIR}/privateer-lib/privateer-export.hpp"
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/privateer/
	COMPONENT Devel
)

configure_package_config_file(Config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/privateer-lib/privateerConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/privateer
	PATH_VARS INCLUDE_INSTALL_DIR LIBRARY_INSTALL_DIR SHARE_INSTALL_DIR
)

install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/privateer-lib/privateerConfig.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/privateer-lib/privateerConfigVersion.cmake"
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/privateer
	COMPONENT Devel
)

set(privateer_MAJOR_VERSION ${CMAKE_PROJECT_VERSION_MAJOR})
set_target_properties(privateer-lib PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${privateer_MAJOR_VERSION}
	INTERFACE_privateer_MAJOR_VERSION ${privateer_MAJOR_VERSION})

set_property(TARGET privateer-lib APPEND PROPERTY
  COMPATIBLE_INTERFACE_STRING privateer_MAJOR_VERSION
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/privateer-lib/privateerConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# pkgconfig support

set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(includedir  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libprivateer.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/libprivateer.pc.in @ONLY)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libprivateer.pc
	INPUT ${CMAKE_CURRENT_BINARY_DIR}/libprivateer.pc.in)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libprivateer.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# The privateer executable

add_executable(privateer-exec ${PRIVATEER_SOURCE_DIR}/cpp/privateer.cpp)
target_link_libraries(privateer-exec privateer-lib)
set_target_properties (privateer-exec PROPERTIES OUTPUT_NAME privateer)

install(TARGETS privateer-exec
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# The python modules

# add_subdirectory(${CMAKE_SOURCE_DIR}/dependencies/pybind11)
find_package(pybind11 REQUIRED)

execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" -c "if True:
    from distutils import sysconfig as sc
    print(sc.get_python_lib(prefix='', plat_specific=True))"
  OUTPUT_VARIABLE PYTHON_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE)

pybind11_add_module(privateer-core
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pybind11.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-ccp4mg.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-restraints.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pyanalysis.cpp
	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pymodelling.cpp)

target_link_libraries(privateer-core PRIVATE privateer-lib)
set_target_properties(privateer-core PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PYTHON_SITE}/privateer
	LIBRARY_OUTPUT_NAME _privateer_core
	PREFIX "")

# pybind11_add_module(privateer-modelling
# 	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pybind11.cpp
# 	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-ccp4mg.cpp
# 	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-restraints.cpp
# 	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pyanalysis.cpp
# 	${PRIVATEER_SOURCE_DIR}/cpp/pybind11/privateer-pymodelling.cpp)

# target_link_libraries(privateer-modelling PRIVATE privateer-lib)
# set_target_properties(privateer-modelling PROPERTIES
# 	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PYTHON_SITE}/privateer
# 	LIBRARY_OUTPUT_NAME _privateer_modelling
# 	PREFIX "")

# install(TARGETS privateer-core privateer-modelling
# 	RUNTIME DESTINATION ${PYTHON_SITE}/privateer
# 	LIBRARY DESTINATION ${PYTHON_SITE}/privateer)

install(FILES ${PRIVATEER_SOURCE_DIR}/__init__.py ${PRIVATEER_SOURCE_DIR}/analysis.py
		${PRIVATEER_SOURCE_DIR}/restraints.py ${PRIVATEER_SOURCE_DIR}/torsions.py
	DESTINATION ${PYTHON_SITE}/privateer)

# install data files, if CLIBD is known

if(CLIBD)
	install(
		FILES
			${PRIVATEERDATA}/glycomics/privateer_glycomics_database.json
			${PRIVATEERDATA}/linkage_torsions/privateer_torsion_database.json
		DESTINATION ${CLIBD})
endif()
