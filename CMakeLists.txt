cmake_minimum_required(VERSION 3.1...3.14)

# Back compatibility for VERSION range
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(gigamonkey 	VERSION 1.0
		DESCRIPTION "open-source Bitcoin library in c++"
		LANGUAGES CXX)

# Set cmake as import path for Find*.cmake files
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

## Check if GTests is installed. If not, install it

option(PACKAGE_TESTS "Build the tests" ON)
if(NOT TARGET gtest_main AND PACKAGE_TESTS)
	# Download and unpack googletest at configure time
	configure_file(cmake/gtests.txt.in googletest-download/CMakeLists.txt)
	execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
			RESULT_VARIABLE result
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
	if(result)
		message(FATAL_ERROR "CMake step for googletest failed: ${result}")
	endif()
	execute_process(COMMAND ${CMAKE_COMMAND} --build .
			RESULT_VARIABLE result
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
	if(result)
		message(FATAL_ERROR "Build step for googletest failed: ${result}")
	endif()

	# Prevent overriding the parent project's compiler/linker
	# settings on Windows
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	
	# Add googletest directly to our build. This defines
	# the gtest and gtest_main targets.
	add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
			${CMAKE_CURRENT_BINARY_DIR}/googletest-build)
endif()


# Check Git submodules

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()


# Include libraries

# Boost settings
set(Boost_LIB_PREFIX            "lib"       CACHE STRING "")
set(Boost_USE_MULTITHREADED     ON          CACHE BOOL "") # '-mt' flag
set(Boost_USE_STATIC_LIBS       ON          CACHE BOOL "")
set(Boost_USE_STATIC_RUNTIME    OFF         CACHE BOOL "") # '-s' tag
set(Boost_USE_DEBUG_RUNTIME     OFF         CACHE BOOL "") # '-g' tag
set(Boost_COMPILER              "-mgw49"    CACHE STRING "")

# Include Boost
find_package(Boost 1.72.0 COMPONENTS system  REQUIRED)

if(Boost_FOUND)

    message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")
    message(STATUS "Boost_VERSION: ${Boost_VERSION}")

    include_directories(${Boost_INCLUDE_DIRS})
    add_definitions("-DHAS_BOOST")

endif()

# Find Crypto++
find_package(CryptoPP REQUIRED)
if(CRYPTOPP_INCLUDE_DIRS)

	message(STATUS "Crypto++ INCLUIDE DIRS: ${CRYPTOPP_INCLUDE_DIRS}")
	include_directories(${CRYPTOPP_INCLUDE_DIRS})
endif()

# Check data library
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/data/CMakeLists.txt")
    message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/bitcoin-sv/CMakeLists.txt")
	message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()
## Enable testing
include(CTest)

if(PACKAGE_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()

add_subdirectory("${PROJECT_SOURCE_DIR}/extern/data/")
add_subdirectory("${PROJECT_SOURCE_DIR}/extern/bitcoin-sv/")
add_library(gigamonkey STATIC 
    src/bitcoin_sv/hash.cpp
    src/bitcoin_sv/signature.cpp
    src/bitcoin_sv/script.cpp
    src/gigamonkey/secp256k1.cpp
    src/gigamonkey/merkle.cpp
    src/gigamonkey/script.cpp
    src/gigamonkey/address.cpp
    src/gigamonkey/wif.cpp
    #src/gigamonkey/spv.cpp
    src/gigamonkey/timechain.cpp
    src/gigamonkey/work.cpp
    src/gigamonkey/redeem.cpp
    src/gigamonkey/schema/hd.cpp
    src/gigamonkey/schema/random.cpp
    src/gigamonkey/wallet.cpp
    src/gigamonkey/stratum/stratum.cpp
    src/gigamonkey/boost/boost.cpp
    #src/bitcoin_sv/sv.cpp 
)

target_link_libraries(gigamonkey PUBLIC data common util bitcoinconsensus)

target_include_directories(gigamonkey PUBLIC include)

# Set C++ version
target_compile_features(gigamonkey PUBLIC cxx_std_17)
set_target_properties(gigamonkey PROPERTIES CXX_EXTENSIONS OFF)
