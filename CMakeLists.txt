cmake_minimum_required (VERSION 3.10)


project (SymuVia VERSION 1.0.0)

# Package installations 
# 
#       brew install boost boost-python3 gdal xqilla rapidjson python
#
# Caveat: 
# 
#       For python 2 
#       
#       brew install boost-python 
# 
#       Also when importing use find_package(Boost COMPONENTS python27 REQUIRED)
# 
# Linux: 
# 
#       apt-get update && apt-get install -y \
#       cmake \
#       xqilla \
#       libboost-all-dev \
#       aptitude \
#       gdal-bin \
#       rapidjson-dev  &&\
#       aptitude search -y \
#       boost 
#
# Boost search
#
# If you have installed via: brew install boost-python 
#
# The following caveat is messaged 
#
#       This formula provides Boost.Python for Python 2. Due to a
#       collision with boost-python3, the CMake Config files are not
#       available. Please use -DBoost_NO_BOOST_CMAKE=ON when building
#       with CMake or switch to Python 3.
#
# Hence when building use: 
#
#       cmake -DBoost_NO_BOOST_CMAKE=ON .. 
#
# Regular use is : 
#
#  cmake ..

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

find_package(Boost COMPONENTS chrono date_time locale log thread serialization wserialization filesystem system python REQUIRED)

    if(Boost_FOUND)
        message(STATUS "Library Boost is available")
        message(STATUS "\t ${Boost_CHRONO_LIBRARY}")
        message(STATUS "\t ${Boost_DATE_TIME_LIBRARY}")
        message(STATUS "\t ${Boost_LOCALE_LIBRARY}")
        message(STATUS "\t ${Boost_SERIALIZATION_LIBRARY}")
        message(STATUS "\t ${Boost_WSERIALIZATION_LIBRARY}")
        message(STATUS "\t ${Boost_PYTHON_LIBRARY}")
        message(STATUS "\t ${Boost_LOG_LIBRARY}")
        message(STATUS "\t ${Boost_FILESYSTEM_LIBRARY}")
        message(STATUS "\t ${Boost_THREAD_LIBRARY}")
        message(STATUS "\t ${Boost_SYSTEM_LIBRARY}")
        include_directories(${Boost_INCLUDE_DIRS})
        message(STATUS "\t Boost headers included")
    else()
        message(STATUS "\t Boost library is missing") 
    endif()

# GDAL search
find_package(GDAL REQUIRED)

    if(GDAL_FOUND)  
        message(STATUS "Library GDAL is available") 
        message(STATUS "\t GDAL library: ${GDAL_LIBRARY},\n \t Headers in: ${GDAL_INCLUDE_DIR}")
        include_directories(${GDAL_INCLUDE_DIR})
        message(STATUS "\t GDAL headers included")
    else()
        message(STATUS "\t GDAL library is missing") 
    endif()

    # GDAL Manual set
    # message(STATUS "GDAL Manually added") 
    # set(GDAL_LIBRARY "/usr/local/Cellar/gdal/2.4.4/lib/libgdal.dylib")
    # set(GDAL_INCLUDE_DIR "/usr/local/include")
    # message(STATUS "\t GDAL library: ${GDAL_LIBRARY},\n \t Headers in: ${GDAL_INCLUDE_DIR}")
    # include_directories(${GDAL_INCLUDE_DIR})
    # message(STATUS "\t GDAL headers included")    

# Xerces Search
find_package(XercesC REQUIRED)

    if(XercesC_FOUND)
        message(STATUS "Library XercesC is available")
        message(STATUS "\t XercesC library: ${XercesC_LIBRARIES},\n \t Headers in: ${XercesC_INCLUDE_DIRS}")
        include_directories(${XercesC_INCLUDE_DIRS})
        message(STATUS "\t XercesC headers included")        
    else()
        message(STATUS "\t XercesC library is missing") 
    endif()

# ODBC Search
find_package(ODBC REQUIRED)

    if(ODBC_FOUND)
        message(STATUS "Library ODBC is available")
        message(STATUS "\t ODBC library: ${ODBC_LIBRARY},\n \t Headers in: ${ODBC_INCLUDE_DIR}")
        include_directories(${ODBC_INCLUDE_DIR})
        message(STATUS "\t ODBC headers included")        
    else()
        message(STATUS "\t ODBC library is missing") 
    endif()

# Python Search
find_package(Python COMPONENTS Interpreter Development REQUIRED)

    if(Python_FOUND)
        include_directories(${Python_INCLUDE_DIRS})
        message(STATUS "Python is available")    
        message(STATUS "\t Python library: ${Python_LIBRARIES},\n \t Headers in: ${Python_INCLUDE_DIRS}")
        message(STATUS "\t Python headers included")         
    else()
        message(STATUS "\t Python library is missing") 
    endif()

# Xqilla/Rapidjson Manual add

    # Libraries 
    if(APPLE)
        set(XQILLA_LIB_DIRECTORY "/usr/local/Cellar/xqilla/2.3.4/lib/libxqilla.dylib" CACHE PATH "Xqilla library folder")
        # set(RAPIDJSON_LIBRARY "/usr/local/lib/" CACHE PATH "Rapidjson library folder")
    else()
        set(XQILLA_LIB_DIRECTORY "/usr/lib/x86_64-linux-gnu/libxqilla.so.6" CACHE PATH "Xqilla library folder")
        # set(RAPIDJSON_LIBRARY "/usr/lib/x86_64-linux-gnu/" CACHE PATH "Rapidjson library folder")
    endif()

    # Dynamic linking 
    # https://stackoverflow.com/questions/26807329/no-rule-to-make-target-error-in-cmake-when-linking-to-shared-library
    # link_directories(${XQILLA_LIB_DIRECTORY})
    # link_directories(${RAPIDJSON_LIBRARY})    

    # Adding headers
    if(APPLE)
        set(XQILLA_INCLUDE_DIR "/usr/local/Cellar/xqilla/2.3.4/include/xqilla/" CACHE PATH "XQilla include folder")
        set(RAPIDJSON_INCLUDE_DIR "/usr/local/Cellar/rapidjson/1.1.0/include/rapidjson/" CACHE PATH "Rapidjson include folder")
    else()
        set(XQILLA_INCLUDE_DIR "/usr/include/" CACHE PATH "XQilla include folder")
        set(RAPIDJSON_INCLUDE_DIR "/usr/include/" CACHE PATH "Rapidjson include folder")
    endif()

    message(STATUS "Xqilla Manually added:")  
    message(STATUS "Xqilla is available at:\n \t Xqilla library: ${XQILLA_LIB_DIRECTORY},\n \t Headers in: ${XQILLA_INCLUDE_DIR}")

    include_directories(${XQILLA_INCLUDE_DIR})
    message(STATUS "\t Rapidjson headers included")  

    message(STATUS "Rapidjson Manually added:")  
    message(STATUS "Rapidjson is available at:\n \t Headers in: ${RAPIDJSON_INCLUDE_DIR}")

    include_directories(${RAPIDJSON_INCLUDE_DIR})
    message(STATUS "\t Rapidjson headers included") 

# Define libraries to link
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH ${LIB_DIRECTORIES})
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Activate C++11 support under Linux
if(CMAKE_COMPILER_IS_GNUCXX OR APPLE)
    list( APPEND CMAKE_CXX_FLAGS "-std=c++0x")
endif()

# Function for printing included files
function(print_files list_files)
    foreach(file IN LISTS "${list_files}")
        message(STATUS "File: ${file}")
    endforeach()
endfunction()


add_subdirectory(SymuCore)
add_subdirectory(SymuVia)
