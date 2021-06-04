project(SymuVia CXX)

set(USE_SYMUCOM OFF CACHE BOOL "Use SymuCom")

# Version control
set(SYMUVIA_VERSION_MAJOR 9)
set(SYMUVIA_VERSION_MINOR 0)
set(SYMUVIA_VERSION_REVISION 0)
set(SYMUVIA_VERSION_BUILD 0)


# Not clear what this is for.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SymuviaVersion.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/src/SymuviaVersion.h
  @ONLY)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Symubruit.rc.in
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Symubruit.rc
  @ONLY)

# Set processor flags
add_definitions(-DTRAFFICSTATE_EXPORTS)
add_definitions(-DSYMUVIA_EXPORTS)
add_definitions(-DUSE_SYMUCORE)
add_definitions(-DBOOST_LOG_DYN_LINK)

if(USE_SYMUCOM)
    add_definitions(-DUSE_SYMUCOM)
endif(USE_SYMUCOM)

file(GLOB SymuVia_src
    "src/*.h"
	"src/*.hpp"
    "src/*.cpp"
    "src/*.c"
    "src/*/*.h"
	"src/*/*.hpp"
    "src/*/*.cpp"
    "src/*/*/*.h"
    "src/*/*/*.cpp"
    "src/*/*/*/*.h"
    "src/*/*/*/*.cpp"
    "src/*.rc"
    "src/*.rgs"
)

# removing historical files not used anymore but keeped in the source code repository
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/CarrefourAFeux.cpp)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/CarrefourAFeux.h)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/StatUtil.cpp)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/StatUtil.h)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/XMLDocReseauCirculation.cpp)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/XMLDocReseauCirculation.h)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/MemoryManagerImpl.cpp)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/MemoryManagerImpl.h)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/Xerces/MemoryManagerImpl.cpp)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/Xerces/MemoryManagerImpl.h)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/SymuScriptManager.cpp)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/SymuScriptManager.h)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/cryptage/tdes.h)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/cryptage/tdes.c)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/cryptage/tdesmain.h)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/cryptage/Cryptage3DES.h)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/cryptage/Cryptage3DES.cpp)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/tests/Parallelisation/main.cpp)
list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/usage/SymuViaTripNodeDebug.cpp)

if(NOT USE_SYMUCOM)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Applications/C_ITSApplication.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Applications/C_ITSApplication.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Applications/C_ITSScriptableApplication.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Applications/C_ITSScriptableApplication.h)
	list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Applications/C_ITSExternalApplication.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Applications/C_ITSExternalApplication.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Applications/DerivedClass/GLOSA.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Applications/DerivedClass/GLOSA.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/ExternalLibraries/ITSExternalLibrary.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/ExternalLibraries/ITSExternalLibrary.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/BubbleSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/BubbleSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/GPSSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/GPSSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/InertialSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/InertialSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSDynamicSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSDynamicSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSScriptableDynamicSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSScriptableDynamicSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSScriptableStaticSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSScriptableStaticSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSExternalDynamicSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSExternalDynamicSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSExternalStaticSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSExternalStaticSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSStaticSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/ITSStaticSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/MagicSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/MagicSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/TelemetrySensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/TelemetrySensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/UBRSensor.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Sensors/DerivedClass/UBRSensor.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/DerivedClass/ITSCAF.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/DerivedClass/ITSCAF.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/ITSExternalStation.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/ITSExternalStation.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/ITSScriptableStation.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/ITSScriptableStation.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/ITSStation.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/ITSStation.h)
	list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/ITSStaticStation.cpp)
	list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/ITSStaticStation.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/DerivedClass/ITSVehicle.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/DerivedClass/ITSVehicle.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/DerivedClass/Simulator.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/DerivedClass/Simulator.h)
	list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/DerivedClass/Simulator.h)
	list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/SymuComSerialization.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/SymuComSerialization.h)
	list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/ITSScriptableElement.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/ITSScriptableElement.h)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/DerivedClass/BMAVehicle.cpp)
    list(REMOVE_ITEM SymuVia_src ${CMAKE_CURRENT_SOURCE_DIR}/src/ITS/Stations/DerivedClass/BMAVehicle.h)
endif(NOT USE_SYMUCOM)

# Files in project
# print_files(SymuVia_src)

# Define SymuCore target: library
add_library(SymuVia SHARED ${SymuVia_src})

set_property(TARGET SymuVia PROPERTY CXX_STANDARD 11)
set_property(TARGET SymuVia PROPERTY CXX_STANDARD_REQUIRED)

# Link Library dependencies
# GDAL
target_link_libraries( SymuVia ${GDAL_LIBRARY})

# SymuCore
target_link_libraries( SymuVia SymuCore)

# Boost
# target_link_libraries( SymuVia ${Boost_SERIALIZATION_LIBRARY})
# target_link_libraries( SymuVia ${Boost_WSERIALIZATION_LIBRARY})
# target_link_libraries( SymuVia ${Boost_PYTHON27_LIBRARY})
# target_link_libraries( SymuVia ${Boost_LOCALE_LIBRARY})
# target_link_libraries( SymuVia ${Boost_DATE_TIME_LIBRARY})
# target_link_libraries( SymuVia ${Boost_LOG_LIBRARY})
# target_link_libraries( SymuVia ${Boost_SYSTEM_LIBRARY})
# target_link_libraries( SymuVia ${Boost_FILESYSTEM_LIBRARY})
# target_link_libraries( SymuVia ${Boost_THREAD_LIBRARY})
# target_link_libraries( SymuVia ${Boost_CHRONO_LIBRARY})
target_link_libraries( SymuVia ${Boost_LIBRARIES})


# XQilla
target_link_libraries( SymuVia ${XQILLA_LIBRARIES})

# Xerces-c
target_link_libraries( SymuVia ${XercesC_LIBRARIES})

# Rapidjson
#target_link_libraries( SymuVia ${RAPIDJSON_LIBRARY}/${RAPIDJSON_LIBRARY})

# Python
target_link_libraries( SymuVia ${Python3_LIBRARIES})

# Visible
target_include_directories(SymuVia PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/reseau.xsd DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

add_executable(SymuViaLauncher src/launcher/SymuViaLauncher.cpp)

set_property(TARGET SymuViaLauncher PROPERTY CXX_STANDARD 11)
set_property(TARGET SymuViaLauncher PROPERTY CXX_STANDARD_REQUIRED)

target_link_libraries(SymuViaLauncher SymuVia)
target_link_libraries(SymuViaLauncher ${Boost_LIBRARIES})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/reseau.xsd DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(TARGETS SymuVia DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(TARGETS SymuViaLauncher DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
