cmake_minimum_required(VERSION 3.2)
project(Libccnx-transport-rta)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if( UNIX )
  link_libraries(m)
endif( UNIX )

include( CTest )
include( version )
include( detectCacheSize )

if(ANDROID_API)
    message("############ Detected cross compile for $ENV{CMAKE_SYSTEM_NAME}")
    message("############ This build will not include doxygen, tools, or tests")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ANDROID_C_FLAGS} -Wall")
else()
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -DNDEBUG -DCCNxTransport_DISABLE_VALIDATION -DLIBRTA_DISABLE_VALIDATION -DTransportLibrary_DISABLE_VALIDATION")
endif()

#set(CMAKE_C_FLAGS_NOPANTS "${CMAKE_C_FLAGS_NOPANTS} -O3 -DNDEBUG -DCCNxTransport_DISABLE_VALIDATION -DLIBRTA_DISABLE_VALIDATION -DTransportLibrary_DISABLE_VALIDATION")

include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_SOURCE_DIR}/ccnx/transport/transport_rta ${PROJECT_BINARY_DIR}/ccnx)
include_directories($ENV{CCNX_DEPENDENCIES}/include)
set(OPENSSL_ROOT_DIR $ENV{CCNX_DEPENDENCIES})

find_package( LongBow REQUIRED )
include_directories(${LONGBOW_INCLUDE_DIRS})

find_package( LibEvent REQUIRED )
include_directories(${LIBEVENT_INCLUDE_DIRS})

find_package( Libparc REQUIRED )
include_directories(${LIBPARC_INCLUDE_DIRS})

find_package( CCNX_Common REQUIRED )
include_directories(${CCNX_COMMON_INCLUDE_DIRS})

find_package ( Threads REQUIRED )

find_package ( OpenSSL REQUIRED )

find_package( Doxygen )

if (ANDROID_API)
    macro(AddTest testFile)
        message("Android build: Skipping test ${ARGV0}")
    endmacro(AddTest)
else()
   macro(AddTest testFile)
     add_executable(${ARGV0} ${ARGV0}.c)
     target_link_libraries(${ARGV0} ${LONGBOW_LIBRARIES})
     target_link_libraries(${ARGV0} ${LIBEVENT_LIBRARIES})
     target_link_libraries(${ARGV0} ${OPENSSL_LIBRARIES})
     target_link_libraries(${ARGV0} ${CMAKE_THREAD_LIBS_INIT})
     target_link_libraries(${ARGV0} ccnx_transport_rta)
     target_link_libraries(${ARGV0} ccnx_api_control)
     target_link_libraries(${ARGV0} ccnx_api_notify)
     target_link_libraries(${ARGV0} ${CCNX_COMMON_LIBRARIES})
     target_link_libraries(${ARGV0} ${LIBPARC_LIBRARIES})
     add_test(${ARGV0} ${ARGV0})
     set_target_properties(${ARGV0} PROPERTIES FOLDER Test)
   endmacro(AddTest)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS} -undefined dynamic_lookup")
	message( "-- Set \"-undefined dynamic_lookup\" for shared libraries")
endif()

add_subdirectory(ccnx)

# Generate DEB / RPM packages

option(DEB_PACKAGE "Create deb package" OFF)
option(RPM_PACKAGE "Create deb package" OFF)

set(VENDOR "Cisco Systems" CACHE STRING "Vendor")
set(CONTACT "msardara@cisco.com" CACHE STRING "Contact")
set(DISTRIBUTION "xenial" CACHE STRING "Distribution")
set(ARCHITECTURE "amd64" CACHE STRING "Architecture")
set(PACKAGE_MAINTAINER "Mauro Sardara (msardara@cisco.com)" CACHE STRING "Maintainer")
string(TOLOWER ${CMAKE_PROJECT_NAME} PACKAGE_NAME)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_PACKAGE_VENDOR ${VENDOR})
set(CPACK_PACKAGE_CONTACT ${CONTACT})
set(CPACK_COMPONENTS_ALL library headers documentation)

# Get the version
execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/version
        OUTPUT_VARIABLE PACKAGE_VERSION)

if (PACKAGE_VERSION)
    string(STRIP ${PACKAGE_VERSION} PACKAGE_VERSION)
else()
    set(PACKAGE_VERSION 1.0)
endif()

if(DEB_PACKAGE)
    set(TYPE "DEBIAN")
    set(GENERATOR "DEB")

    set(CPACK_${TYPE}_LIBRARY_PACKAGE_NAME "${PACKAGE_NAME}")
    set(CPACK_${TYPE}_HEADERS_PACKAGE_NAME "${PACKAGE_NAME}-dev")
    set(CPACK_${TYPE}_DOCUMENTATION_PACKAGE_NAME "${PACKAGE_NAME}-doc")

    set(CPACK_${TYPE}_LIBRARY_FILE_NAME "${PACKAGE_NAME}_${PACKAGE_VERSION}_${ARCHITECTURE}.deb")
    set(CPACK_${TYPE}_HEADERS_FILE_NAME "${PACKAGE_NAME}-dev_${PACKAGE_VERSION}_${ARCHITECTURE}.deb")
    set(CPACK_${TYPE}_DOCUMENTATION_FILE_NAME "${PACKAGE_NAME}-doc_${PACKAGE_VERSION}_${ARCHITECTURE}.deb")

    set(CPACK_DEBIAN_LIBRARY_PACKAGE_SHLIBDEPS ON)

    set(CPACK_${TYPE}_LIBRARY_PACKAGE_DEPENDS "longbow (>= 1.0), libparc (>= 1.0), libccnx-common (>= 1.0)")
    set(CPACK_${TYPE}_HEADERS_PACKAGE_DEPENDS "libccnx-transport-rta (>= 1.0), libccnx-common-dev (>= 1.0), libparc-dev (>= 1.0), longbow-dev (>= 1.0)")
    set(CPACK_${TYPE}_DOCUMENTATION_PACKAGE_DEPENDS "")
elseif(RPM_PACKAGE)
    set(TYPE "RPM")
    set(GENERATOR "RPM")

    set(CPACK_${TYPE}_LIBRARY_PACKAGE_NAME "${PACKAGE_NAME}")
    set(CPACK_${TYPE}_HEADERS_PACKAGE_NAME "${PACKAGE_NAME}-devel")
    set(CPACK_${TYPE}_DOCUMENTATION_PACKAGE_NAME "${PACKAGE_NAME}-doc")

    set(CPACK_${TYPE}_LIBRARY_FILE_NAME "${PACKAGE_NAME}-${PACKAGE_VERSION}.${ARCHITECTURE}.rpm")
    set(CPACK_${TYPE}_HEADERS_FILE_NAME "${PACKAGE_NAME}-devel-${PACKAGE_VERSION}.${ARCHITECTURE}.rpm")
    set(CPACK_${TYPE}_DOCUMENTATION_FILE_NAME "${PACKAGE_NAME}-doc-${PACKAGE_VERSION}.${ARCHITECTURE}.rpm")

    set(CPACK_${TYPE}_LIBRARY_PACKAGE_AUTOREQ ON)

    set(CPACK_${TYPE}_LIBRARY_PACKAGE_REQUIRES "longbow >= 1.0, libparc >= 1.0, libccnx-common >= 1.0")
    set(CPACK_${TYPE}_HEADERS_PACKAGE_REQUIRES "libccnx-transport-rta >= 1.0, libccnx-common-devel >= 1.0, libparc-devel >= 1.0, longbow-devel >= 1.0")
    set(CPACK_${TYPE}_DOCUMENTATION_PACKAGE_REQUIRES "")

    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/etc" "/usr/lib/python2.7" "/usr/lib/python2.7/site-packages")
else()
    return()
endif()

set(CPACK_GENERATOR ${GENERATOR})
set(CPACK_${GENERATOR}_COMPONENT_INSTALL ON)
set(CPACK_${TYPE}_PACKAGE_MAINTAINER ${PACKAGE_MAINTAINER})
set(CPACK_${TYPE}_PACKAGE_NAME ${PACKAGE_NAME})
set(CPACK_${TYPE}_PACKAGE_VERSION ${PACKAGE_VERSION})
set(CPACK_${TYPE}_PACKAGE_ARCHITECTURE ${ARCHITECTURE})
set(CPACK_${TYPE}_PACKAGE_RELEASE 1)
set(CPACK_${TYPE}_PACKAGE_VENDOR ${VENDOR})
set(CPACK_${TYPE}_PACKAGE_DESCRIPTION "The CCNx RTA Stack implementation.")
set(CPACK_${TYPE}_PACKAGE_HOMEPAGE "https://wiki.fd.io/view/Ccnxlib")

include(CPack)
