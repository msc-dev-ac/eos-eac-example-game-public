#CMAKE file for build
cmake_minimum_required(VERSION 2.9)
project (Lobbies)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release) #default type is release
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include_directories ("${PROJECT_SOURCE_DIR}/src")

#EOS SDK location

set(EOS_SDK_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/SDK/Include")
set(EOS_SDK_BIN_DIR "${PROJECT_SOURCE_DIR}/SDK/Bin")

set(EOS_SDK_LIBRARIES "${EOS_SDK_BIN_DIR}/libEOSSDK-Linux-Shipping.so") #linux

include_directories ("${PROJECT_SOURCE_DIR}/src")
include_directories ("${EOS_SDK_INCLUDE_DIR}")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(THIRD_PARTY_DEPENDENCY_LIBS "")
# help out poor VSCode
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

#Search for dependencies

set(SOURCES
	src/main.cpp
    src/eac.cpp
    src/comms.cpp
)

set(APP_NAME MyEOSGameServer)

add_executable(${APP_NAME} ${SOURCES})

target_link_libraries(${APP_NAME} ${THIRD_PARTY_DEPENDENCY_LIBS} ${EOS_SDK_LIBRARIES})

set_property(TARGET ${APP_NAME} PROPERTY C_STANDARD 99)
set_property(TARGET ${APP_NAME} PROPERTY CXX_STANDARD 14)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Destination paths below are relative to ${CMAKE_INSTALL_PREFIX}
install(TARGETS ${APP_NAME}
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime)

# Set platform-specific path to executable
set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/${APP_NAME}")

# Directories to look in for dependencies
set(DIRS ${EOS_SDK_BIN_DIR}
	)

install(CODE "include(BundleUtilities)
    fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\")")


